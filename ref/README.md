# Local reference (not shipped)

Use this folder for **upstream source trees you clone locally** — design reference and learning only. Example:

```bash
git clone https://github.com/id-Software/Quake.git ref/Quake
```

Everything under `ref/` except this `README.md` is **gitignored** so GPL (or other) reference code never enters the Greywater repo history or release artifacts.

Do **not** copy large verbatim blocks from reference trees into `engine/` or `gameplay/` without explicit legal review (copyleft and license boundaries).
