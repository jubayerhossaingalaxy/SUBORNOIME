# MCP Integration

The workspace MCP configuration is `.mcp.json`.

`microsoftLearn` connects to `https://learn.microsoft.com/api/mcp` for official
Microsoft documentation lookup. It is read-only in the initial integration.

No local MCP server is enabled. A server must not receive shell, delete,
credential or write permissions during initial setup.

GitHub MCP or built-in GitHub tools require no additional project server
configuration. SQLite, CMake and filesystem MCPs are not needed because native
APIs and existing VS Code tools already cover those needs.
