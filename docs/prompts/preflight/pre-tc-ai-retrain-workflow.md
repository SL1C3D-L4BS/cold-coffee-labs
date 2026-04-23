# pre-tc-ai-retrain-workflow - Nightly AI retrain workflow

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | B |
| Depends on | *(none)* |

## Inputs

- `tools/cook/ai/`

## Writes

- `.github/workflows/ai_retrain.yml`

## Exit criteria

- [ ] Scheduled workflow invokes each pipeline and uploads model artifacts

## Prompt

Create ai_retrain.yml (cron nightly). Matrix over five pipelines;
each step runs the pipeline's entry script and uploads artifacts.
Opens a GitHub issue if the model hash changes.

