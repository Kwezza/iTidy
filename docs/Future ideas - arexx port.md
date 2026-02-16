# Future ideas - ARexx port (iTidy)

## Summary
Add an optional ARexx control port to iTidy so power users can automate tidy runs and DefIcons-based permanent icon creation over one or more drawers.

This is intentionally a small v1 feature to "test the water" and expand only if users actually adopt it.

## Goals
- Keep the ARexx API small and stable.
- Single instance, single job at a time.
- No background footprint unless the user enables the port.
- ARexx-triggered jobs can show a progress window.
- Provide `STATUS` and `CANCEL` via ARexx while a job is running.

## Non-goals (v1)
- Multi-job queueing.
- Multiple simultaneous iTidy instances with unique ports.
- Full exposure of every GUI option as ARexx arguments.
- Headless operation (may be added later).

## User experience
### Main window menu
Add a main menu entry:

**ARexx**
- Enable ARexx port (toggle)
- Port name (default `ITIDY`) (optional for v1, fixed name is acceptable)

Optional preference:
- Start with ARexx enabled (off by default)

### Behaviour
- If ARexx is disabled, no public port exists.
- When ARexx is enabled, iTidy creates and listens on a public port (default `ITIDY`).
- ARexx-triggered jobs open the standard progress window.
- While a job is running, the ARexx interface responds to `STATUS` and `CANCEL`.

## Operational model
### Single-instance server inside the GUI app
- iTidy runs normally as a GUI app.
- ARexx, when enabled, is just an additional input channel (message port).
- Commands are received quickly, parsed, validated, then either:
  - start a background job and reply immediately, or
  - return an error if iTidy is busy.

### Busy semantics
- iTidy accepts only one job at a time.
- Starting a new job while busy returns `ERR 20 BUSY`.
- `STATUS` and `CANCEL` must work while busy.

## Profiles
Use existing "Save settings" profiles as the main configuration mechanism.

- ARexx commands accept `PROFILE="<name>"`.
- If `PROFILE` is omitted, use the current GUI profile or a defined default.
- Profiles should be treated as an input to a "job settings" struct, not something that mutates the user's current GUI settings.

Errors:
- Missing profile: `ERR 11 PROFILE_NOT_FOUND`
- Invalid path: `ERR 10 PATH_NOT_FOUND`

## Command spec (v1 draft)
### Reply format
All replies are single-line:

- Success: `OK ...`
- Error: `ERR <code> <message>`

Recommended error codes:
- `10 PATH_NOT_FOUND`
- `11 PROFILE_NOT_FOUND`
- `20 BUSY`
- `21 NOT_BUSY`
- `30 BAD_ARGS`
- `40 INTERNAL_ERROR`

### Commands
#### `PING`
Checks the port and returns version info.

Example:
- Command: `PING`
- Reply: `OK ITIDY <version>`

#### `STATUS`
Reports whether iTidy is idle or busy, and returns basic progress fields.

Replies:
- Idle: `OK IDLE`
- Busy: `OK BUSY JOB=<id> MODE=<TIDY|DEFICONS> PATH="<path>" PROGRESS=<0..100> CREATED=<n> WARN=<n> ERRORS=<n> CUR="<short name>"`

Notes:
- Keep status fields stable and short.
- `CUR` should be a short name, not a full long path, unless requested in a future extension.

#### `CANCEL`
Requests cancellation of the active job.

Replies:
- Busy: `OK CANCELLING`
- Idle: `ERR 21 NOT_BUSY`

Implementation:
- Set a global or shared `cancel_requested` flag and have the worker check it frequently.

#### `TIDY`
Starts a tidy run using a saved profile.

Command:
- `TIDY PATH="<drawer>" PROFILE="<name>" [RECURSE=YES|NO] [INTERACTIVE=YES|NO]`

Defaults:
- `RECURSE=NO`
- `INTERACTIVE=YES` (shows progress window)

Replies:
- Accepted: `OK STARTED JOB=<id>`
- Busy: `ERR 20 BUSY`
- Bad args: `ERR 30 BAD_ARGS`

#### `DEFICONS`
Runs the DefIcons integration to create permanent icons over a drawer (optional recursion).

Command:
- `DEFICONS PATH="<drawer>" PROFILE="<name>" [RECURSE=YES|NO] [INTERACTIVE=YES|NO]`

Replies:
- Accepted: `OK STARTED JOB=<id>`
- Busy: `ERR 20 BUSY`

#### Optional `QUIT` (consider after v1)
Allows scripts to shut down iTidy after the job completes.

Command:
- `QUIT [FORCE=YES|NO]`

Replies:
- If busy and `FORCE=NO`: `ERR 20 BUSY`
- Otherwise: `OK QUITTING`

If `QUIT` feels risky for v1, omit it.

## Progress window behaviour
- Any ARexx-started job opens the existing progress window.
- The progress window should be updated from the same progress data that `STATUS` reads.
- The progress window should allow local user cancellation too, which maps to the same cancel flag used by `CANCEL`.

## Implementation notes (C89, Amiga style)
### Message port
- Create a public message port when ARexx is enabled.
- Drain the port quickly and avoid running long work inside the message handler.

### Main loop integration
In the main Reaction event loop, wait on:
- window signals
- ARexx port signal
- worker/job signal (optional)

On ARexx signal:
- Read messages
- Parse and validate
- Start job if applicable
- Reply immediately with `OK STARTED` or an `ERR`

### Worker job model
- The tidy/DefIcons operation runs as a job, not in the ARexx handler.
- Maintain a small shared status struct for:
  - state (IDLE/RUNNING/CANCELLING)
  - job id
  - mode
  - path
  - progress (percent or done/total)
  - counts (created icons, warnings, errors)
  - current item short name

### Cancellation
- `CANCEL` sets `cancel_requested=TRUE`.
- Worker checks the flag every N items and exits cleanly:
  - close files
  - finalise logs
  - update status to IDLE

### Enabling and disabling the port
- Enable: create port and start listening immediately.
- Disable:
  - If idle: delete the port immediately.
  - If busy: either block disable, or schedule "disable after job completes" (preferred for user experience).

## Risks and mitigations
### API becomes a commitment
Mitigation:
- Keep the command set minimal for v1.
- Add new commands without changing existing behaviour.
- Consider adding `API=<n>` or a `VERSION` response field later.

### Busy and concurrency issues
Mitigation:
- Single-instance, one job at a time.
- Clear `ERR 20 BUSY` behaviour.

### Requesters break scripting
Mitigation:
- Default `INTERACTIVE=YES` for v1 so the user can see and respond.
- For future headless use, add `INTERACTIVE=NO` and guarantee no requesters in that mode.

## Future extensions (only if users ask)
- `LISTPROFILES` and `PROFILEINFO`
- Job queue and `JOBLIST`
- `RESULT` command to fetch the final summary of the last job
- Headless mode with log file output
- Auto-start server mode (`STARTSERVER`) for scripts that want to launch iTidy on demand
