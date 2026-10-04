# pixelelated splash

Framebuffer splash renderer forked from [ROCKNIX/rocknix-splash](https://github.com/ROCKNIX/rocknix-splash).
The renderer and executable name are retained; the upstream logo is replaced
with pixelelated's interim wordmark, without an icon (distribution #337).

`tools/pixelelated-wordmark` regenerates the SVG and C data from the unmodified
Tiny5 Duo LCD font. Its authors and SIL Open Font License1.1 are preserved in
`fonts/OFL.txt`. Ocean Bands uses the canonical RGB555 palette from
distribution D-WORKFLOW-146. The generator intersects each LCD cell with
five bands and emits ordinary colored paths for limited SVG readers, plus
matching C data. No clip/mask support is required at runtime. The renderer
samples pixel centers without blending so cell gaps and hard bands survive.
Build with `make`; test in the distribution's VM.

pixelelated and its artwork follow the distribution's `TRADEMARK.md` and
branding terms. The font remains under its own licence, and the inherited
renderer retains its upstream ownership and licensing.
