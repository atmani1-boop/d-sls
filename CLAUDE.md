# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with this repository.

## Project Overview

d-sls is a project that includes GitHub custom agent configuration. The repository contains a custom agent template in `.github/agents/`.

## Repository Structure

```
.github/
  agents/
    my-agent.agent.md    # Custom agent configuration template
README.md                 # Project documentation
CLAUDE.md                 # This file - Claude Code guidance
```

## Development Guidelines

- Keep the repository structure clean and organized
- Document any new agents or configurations added to `.github/agents/`
- Follow GitHub's custom agent format specification: https://gh.io/customagents/config

## GitHub Custom Agents

The project uses GitHub custom agents. Key resources:
- Agent configuration location: `.github/agents/`
- Local testing via Copilot CLI: https://gh.io/customagents/cli
- Agents become available after merging to the default branch

## Common Tasks

- **Add new agent**: Create a new `.agent.md` file in `.github/agents/` following the YAML frontmatter format
- **Test agents locally**: Use the GitHub Copilot CLI for local testing before merging
