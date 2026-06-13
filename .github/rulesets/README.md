# Rulesets

GitHub **rulesets are not auto-applied from the repo** (unlike Actions workflows). These JSON files are the **source of truth** — reusable by importing or syncing via API.

## Can rulesets be reusable?

| Approach | Reusable? | How |
|----------|-----------|-----|
| **Org rulesets** (`org/*.json`) | ✅ Best | One ruleset targets `~ALL` repos or name patterns. Change once, applies everywhere. |
| **Repo rulesets** (`repo/*.json`) | ✅ Per repo | Copy template → import when creating a new repo. |
| **This folder + script** | ✅ Yes | Same JSON applied to many repos via `apply-rulesets.sh`. |
| **GitHub UI import** | ✅ Yes | Settings → Rules → Rulesets → **Import a ruleset** (pick a JSON file). |
| **Like reusable workflows** | ❌ No | No native `uses: ./rulesets/foo.json` on push. Must import or run the apply script. |

For multi-repo setups: prefer **org-level rulesets** in `org/` and delete or skip `repo/` duplicates to avoid double enforcement.

## What's included

| File | Scope | Protects |
|------|-------|----------|
| `repo/main-branch.json` | Repo | `main` — PR, CI checks, no delete/force-push |
| `repo/release-branch.json` | Repo | `release` — stricter PR rules |
| `repo/protect-tags.json` | Repo | Release tags — no delete/force-push |
| `repo/block-secrets-push.json` | Repo | Blocks `.env`, keys, credentials in pushes |
| `org/default-branches.json` | Org | All repos — default branch + `main` + `release` |
| `org/protect-tags.json` | Org | All repos — tags |
| `org/block-secrets-push.json` | Org | All repos — secret file paths |

Status checks expect **`CI / test`** and **`CI / security`** from [ci.yml](../workflows/ci.yml). Rename in JSON if your workflow job names differ.

## Apply methods

### 1. GitHub UI (simplest)

1. Repo or org **Settings → Rules → Rulesets**
2. **New ruleset → Import a ruleset**
3. Select a JSON file from `repo/` or `org/`
4. Review and save

### 2. Script (repeatable / many repos)

```bash
# Repo-level (admin on the repository)
./scripts/apply-rulesets.sh repo

# Org-level (org owner — applies to all matching repos)
./scripts/apply-rulesets.sh org

# Test first
./scripts/apply-rulesets.sh repo --dry-run
./scripts/apply-rulesets.sh repo --evaluate   # evaluate mode, then switch to active in UI
```

Requires [GitHub CLI](https://cli.github.com/) and `jq`.

The script **upserts by ruleset name** — safe to re-run after editing JSON.

### 3. GitHub Actions

Run **Apply Rulesets** workflow (`workflow_dispatch`):

- **scope**: `repo` or `org`
- **mode**: `evaluate` (test) or `active`
- **dry_run**: preview only

Uses a **`RULESETS_PAT`** secret (fine-grained or classic PAT with `administration: write` on the repo; add org admin for `org` scope). `GITHUB_TOKEN` cannot manage rulesets — it has no `administration` workflow permission.

## After applying — required manual step

Add **GitHub Actions** to the ruleset **bypass list** for branches it must push to (e.g. `prepare-release.yml` pushing to `release`, `start-pull-request.yml` creating branches):

1. Open the ruleset in Settings
2. **Bypass list → Add bypass**
3. Choose **GitHub App → GitHub Actions**

Without this, automated release/PR workflows may be blocked.

## Customize

- Edit JSON files, then re-run the script or workflow
- Adjust `manifest.json` to enable/disable which rulesets get applied
- Tighten `org/default-branches.json` `repository_name.include` from `~ALL` to specific patterns, e.g. `["my-app-*"]`
- See also: [github/ruleset-recipes](https://github.com/github/ruleset-recipes)

## Evaluate vs active

Start with `--evaluate` or workflow mode **evaluate** to test impact without blocking contributors. Switch to **active** in the UI when satisfied.
