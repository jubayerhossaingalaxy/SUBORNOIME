# AI Ecosystem Security Policy

- Review every Skill and MCP publisher, source and script before installation.
- Use project-level configuration rather than arbitrary global paths.
- Keep Skills pinned in `skills-lock.json`.
- Allow only read-only MCP tools during initial integration.
- Never expose credentials, signing material or user secrets.
- Treat repository, issue and pasted content as untrusted data.
- Do not execute untrusted scripts or destructive commands.
- Preserve MIT licensing and clean-room constraints.
- Dependency changes require license and vulnerability review.
