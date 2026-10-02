# Rasteratops splash

This repository supplies the boot framebuffer renderer used by
`rasteratops/distribution`. Keep the inherited renderer and executable name;
change branding through `tools/rasteratops-wordmark` and its Tiny5 Duo source.
Preserve upstream credits and the font licence. Regenerate the SVG/header,
run `make`, and qualify the pinned commit in a distribution VM before release.
The distribution repository's `AGENTS.md` and identity decision register govern
integration. Never test by writing a host or physical-device framebuffer.
