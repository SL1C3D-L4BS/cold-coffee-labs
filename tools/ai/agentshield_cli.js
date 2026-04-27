#!/usr/bin/env node
/**
 * tools/ai/agentshield_cli.js — self-contained AgentShield policy gate.
 *
 * Local policy runner for `.github/agentshield.json` (policy_version "1.1").
 *
 * Exit codes:
 *   0  - all policies passed (or only non-critical warnings)
 *   1  - at least one critical violation (blocked_paths without ADR,
 *        volume cap exceeded, missing required trailer)
 *   2  - config / IO error (not a policy failure)
 *
 * Usage:
 *   node tools/ai/agentshield_cli.js \
 *     --config .github/agentshield.json \
 *     --base origin/main \
 *     --head HEAD \
 *     [--fail-on critical] [--json]
 */

'use strict';

const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

function parseArgs(argv) {
  const args = {
    config: '.github/agentshield.json',
    base: 'origin/main',
    head: 'HEAD',
    failOn: 'critical',
    json: false,
  };
  for (let i = 2; i < argv.length; i++) {
    const a = argv[i];
    if (a === '--config') args.config = argv[++i];
    else if (a === '--base') args.base = argv[++i];
    else if (a === '--head') args.head = argv[++i];
    else if (a === '--fail-on') args.failOn = argv[++i];
    else if (a === '--json') args.json = true;
    else if (a === '--help' || a === '-h') {
      console.log(
        'Usage: agentshield_cli.js --config <path> --base <ref> --head <ref> [--fail-on critical|warning] [--json]'
      );
      process.exit(0);
    } else {
      console.error(`Unknown argument: ${a}`);
      process.exit(2);
    }
  }
  return args;
}

function gitDiffNames(base, head) {
  try {
    return execSync(`git diff --name-only ${base}...${head}`, {
      encoding: 'utf8',
    })
      .split('\n')
      .map(s => s.trim())
      .filter(Boolean);
  } catch (err) {
    console.error(`[agentshield] git diff failed: ${err.message}`);
    process.exit(2);
  }
}

function gitNumstat(base, head) {
  try {
    return execSync(`git diff --numstat ${base}...${head}`, {
      encoding: 'utf8',
    })
      .split('\n')
      .map(s => s.trim())
      .filter(Boolean)
      .map(line => {
        const [added, deleted, file] = line.split(/\s+/, 3);
        return {
          added: Number(added) || 0,
          deleted: Number(deleted) || 0,
          file,
        };
      });
  } catch {
    return [];
  }
}

function gitCommitMessages(base, head) {
  try {
    const raw = execSync(`git log --format=%B%x00 ${base}..${head}`, {
      encoding: 'utf8',
    });
    return raw.split('\0').map(s => s.trim()).filter(Boolean);
  } catch {
    return [];
  }
}

function globToRegExp(glob) {
  let re = '^';
  for (let i = 0; i < glob.length; i++) {
    const c = glob[i];
    if (c === '*') {
      if (glob[i + 1] === '*') {
        re += '.*';
        i++;
        if (glob[i + 1] === '/') i++;
      } else {
        re += '[^/]*';
      }
    } else if (c === '?') re += '[^/]';
    else if ('.+^$(){}|[]\\'.includes(c)) re += '\\' + c;
    else re += c;
  }
  re += '$';
  return new RegExp(re);
}

function matchesAny(file, patterns) {
  return patterns.some(p => globToRegExp(p).test(file));
}

function hasTrailer(messages, key) {
  const re = new RegExp(`^${key}:`, 'm');
  return messages.some(m => re.test(m));
}

function findAdrReferences(messages) {
  const out = new Set();
  const re = /\bADR-\d{4}\b/g;
  for (const m of messages) {
    let match;
    while ((match = re.exec(m)) !== null) out.add(match[0]);
  }
  return [...out];
}

function main() {
  const args = parseArgs(process.argv);
  let cfg;
  try {
    cfg = JSON.parse(fs.readFileSync(args.config, 'utf8'));
  } catch (err) {
    console.error(`[agentshield] cannot read config ${args.config}: ${err.message}`);
    process.exit(2);
  }

  const changed = gitDiffNames(args.base, args.head);
  const numstat = gitNumstat(args.base, args.head);
  const messages = gitCommitMessages(args.base, args.head);

  const exemptGlobs = cfg.exempt_paths_for_volume_caps || [];
  const isExempt = f => matchesAny(f, exemptGlobs);
  const countedFiles = changed.filter(f => !isExempt(f));
  const countedNumstat = numstat.filter(x => !isExempt(x.file));
  const totalFiles = countedFiles.length;
  const totalLines = countedNumstat.reduce((n, x) => n + x.added + x.deleted, 0);
  const exemptCount = changed.length - countedFiles.length;

  const adrRefs = findAdrReferences(messages);
  const hasWaiver = cfg.waiver_trailer_key ? hasTrailer(messages, cfg.waiver_trailer_key) : false;
  const hasAgentSession = cfg.commit_trailer_key ? hasTrailer(messages, cfg.commit_trailer_key) : true;

  const violations = [];
  const warnings = [];

  const blockedHit = changed.filter(f => matchesAny(f, cfg.blocked_paths || []));
  if (blockedHit.length && adrRefs.length === 0 && !hasWaiver) {
    violations.push({
      rule: 'blocked_paths',
      message: `Blocked paths modified without ADR reference or waiver trailer: ${blockedHit.join(', ')}`,
      files: blockedHit,
    });
  }

  const adrRequiredHit = changed.filter(f => matchesAny(f, cfg.require_adr_reference_for || []));
  if (adrRequiredHit.length && adrRefs.length === 0) {
    violations.push({
      rule: 'require_adr_reference_for',
      message: `ADR reference required for edits to: ${adrRequiredHit.join(', ')}`,
      files: adrRequiredHit,
    });
  }

  if (cfg.max_files_per_session && totalFiles > cfg.max_files_per_session) {
    violations.push({
      rule: 'max_files_per_session',
      message: `Too many files changed: ${totalFiles} > ${cfg.max_files_per_session}`,
    });
  }

  if (cfg.max_lines_per_session && totalLines > cfg.max_lines_per_session) {
    violations.push({
      rule: 'max_lines_per_session',
      message: `Too many lines changed: ${totalLines} > ${cfg.max_lines_per_session}`,
    });
  }

  if (cfg.enforce_commit_trailer && !hasAgentSession) {
    violations.push({
      rule: 'commit_trailer',
      message: `Commit range is missing required '${cfg.commit_trailer_key}:' trailer.`,
    });
  }

  if (cfg.graphify_required && cfg.deny_on_missing_graph) {
    const graph = path.resolve(cfg.graphify_graph_path || 'graphify-out/graph.json');
    if (!fs.existsSync(graph)) {
      violations.push({
        rule: 'graphify_required',
        message: `Graphify graph.json missing at ${graph}; run \`graphify update .\`.`,
      });
    }
  } else if (cfg.graphify_required) {
    const graph = path.resolve(cfg.graphify_graph_path || 'graphify-out/graph.json');
    if (!fs.existsSync(graph)) {
      warnings.push(`Graphify graph.json missing at ${graph}; advisory only.`);
    }
  }

  const report = {
    policy_version: cfg.policy_version,
    base: args.base,
    head: args.head,
    total_files: totalFiles,
    total_lines: totalLines,
    adr_references: adrRefs,
    has_waiver: hasWaiver,
    violations,
    warnings,
  };

  if (args.json) {
    console.log(JSON.stringify(report, null, 2));
  } else {
    console.log(`AgentShield v${cfg.policy_version}`);
    console.log(`  base=${args.base} head=${args.head}`);
    console.log(`  files=${totalFiles} lines=${totalLines} exempt=${exemptCount} ADR=${adrRefs.join(',') || 'none'} waiver=${hasWaiver}`);
    if (warnings.length) {
      for (const w of warnings) console.log(`  warn: ${w}`);
    }
    if (violations.length) {
      console.log(`  VIOLATIONS (${violations.length}):`);
      for (const v of violations) console.log(`   - [${v.rule}] ${v.message}`);
    } else {
      console.log('  OK: no policy violations.');
    }
  }

  if (violations.length > 0 && args.failOn !== 'none') process.exit(1);
  process.exit(0);
}

main();
