#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

required_files=(
  ".github/copilot-instructions.md"
  ".github/instructions/cpp.instructions.md"
  ".github/instructions/windows.instructions.md"
  ".github/instructions/tests.instructions.md"
  ".github/instructions/docs.instructions.md"
  ".mcp.json"
  "skills-lock.json"
)

for file in "${required_files[@]}"; do
  test -s "$file" || {
    printf 'FAIL missing or empty: %s\n' "$file" >&2
    exit 1
  }
done

python3 - <<'PY'
import json
from pathlib import Path

lock = json.loads(Path("skills-lock.json").read_text(encoding="utf-8"))
assert lock.get("version") == 2
assert isinstance(lock.get("skills"), dict)

mcp = json.loads(Path(".mcp.json").read_text(encoding="utf-8"))
server = mcp["mcpServers"]["microsoftLearn"]
assert server["type"] == "http"
assert server["url"] == "https://learn.microsoft.com/api/mcp"
PY

agent_count="$(find .github/agents -maxdepth 1 -type f -name '*.agent.md' | wc -l)"
doc_count="$(find docs/ai -maxdepth 1 -type f -name '*.md' | wc -l)"

test "$agent_count" -eq 9 || {
  printf 'FAIL expected 9 custom agents, found %s\n' "$agent_count" >&2
  exit 1
}
test "$doc_count" -eq 11 || {
  printf 'FAIL expected 11 AI docs, found %s\n' "$doc_count" >&2
  exit 1
}

for agent in .github/agents/*.agent.md; do
  test -s "$agent" || exit 1
  grep -q '^tools:.*read.*search' "$agent" || {
    printf 'FAIL agent is missing read/search-only tools: %s\n' "$agent" >&2
    exit 1
  }
done

for doc in docs/ai/*.md; do
  test -s "$doc" || exit 1
done

printf 'PASS AI ecosystem configuration files are present and valid JSON structures parse.\n'
