# Deployment (optional)

These workflows **do not run** until you copy them into `.github/workflows/`.

Pick **one** provider (or copy only what you need).

## Setup

1. Copy workflow(s) to `.github/workflows/`:
   - `deploy-cloudflare.yml`
   - `deploy-vercel.yml`
   - `deploy-hetzner.yml` **and** `reusable-deploy-docker.yml` (Hetzner needs both)
2. Copy infra to repo root:
   - `infra/cloudflare/` ← from `examples/infra/cloudflare/`
   - `infra/docker/` ← from `examples/infra/docker/` (Hetzner only)
3. Add GitHub secrets (see main [README](../../README.md#required-secrets))
4. Create `staging` and `production` environments (optional approval on production)

## Files

| Workflow | Infra | Secrets |
|----------|-------|---------|
| `deploy-cloudflare.yml` | `wrangler.toml` from `infra/cloudflare/wrangler.toml.example` | `CLOUDFLARE_API_TOKEN`, `CLOUDFLARE_ACCOUNT_ID` |
| `deploy-vercel.yml` | — | `VERCEL_TOKEN`, `VERCEL_ORG_ID`, `VERCEL_PROJECT_ID` |
| `deploy-hetzner.yml` | `infra/docker/Dockerfile`, `compose.yml` | `SSH_PRIVATE_KEY`, `DEPLOY_HOST`, `DEPLOY_USER` |

Until you have an app and secrets configured, leave these in `examples/` — CI alone is enough.
