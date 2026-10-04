# pixelelated splash

This repository supplies the boot framebuffer renderer used by
`pixelelated/distribution`. Keep the inherited renderer and executable name;
change branding through `tools/pixelelated-wordmark` and its Tiny5 Duo LCD source.
Preserve upstream credits and the font licence. Regenerate the SVG/header,
run `make`, and qualify the pinned commit in a distribution VM before release.
The distribution repository's `AGENTS.md` and identity decision register govern
integration. Never test by writing a host or physical-device framebuffer.

Resume from `/workspace/repos/rocknix` on `next`: read `AGENTS.md`,
`.github/sessions/saved-session-state-next.md`, and its every-session rules.
The canonical checkpoint owns current priorities, integration and source pins;
this repository keeps an entry pointer, not a second policy corpus
(D-WORKFLOW-010). Read scoped packaging and identity rules before integration.
