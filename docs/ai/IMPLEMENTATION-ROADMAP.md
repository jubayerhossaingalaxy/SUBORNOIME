# AI Ecosystem Implementation Roadmap

## Phase 1: Instructions

Central and path-specific Copilot instructions are now planned and are
discovered through `.github/copilot-instructions.md` and
`.github/instructions/`.

## Phase 2: Agents

Six essential agents and three optional specialists are project-local.
All initial agent definitions are read-only.

## Phase 3: MCP

The Microsoft Learn MCP endpoint is workspace-configured for read-only official
documentation retrieval. No MCP write permission is enabled.

## Phase 4: Verification

Run `scripts/verify-ai-ecosystem.sh`, then inspect VS Code/Copilot discovery.

## Phase 5: CI

The read-only AI ecosystem workflow runs file and JSON structure verification.
Source build behavior remains covered by existing Ubuntu and Windows workflows.

## Phase 6: Future expansion

Add new Skills, agents or MCP servers only after source and security review.
Do not add RAG/vector infrastructure until a measured repository retrieval need
exists.
