# Look workshop

Previews of every Film Look on your own GR DNG files, as one page to compare
them before choosing recipes. The page is published as a claude.ai artifact;
these scripts rebuild its images.

1. Publish the preview tables (needs the GR III 2.10 firmware, not in this
   repository):

   ```sh
   grkit effects publish --source effects --firmware fwdc239b.bin --out site-effects/gr3-2.10
   ```

2. Render (needs Python with numpy, Pillow and rawpy):

   ```sh
   GRKIT_PREVIEW_EFFECTS=site-effects/gr3-2.10 python3 extras/look-workshop/workshop_render.py PHOTOS_DIR OUT_DIR
   ```

`render.py` decodes a DNG as grkit.app does (LibRaw: half size, camera white
balance, linear sRGB, 16 bits, no auto brightness), then applies an effect's
33-point preview table through the sRGB shaper.

## Calibration

LibRaw's decode is darker than the camera's JPEG. One gain per camera closes
most of it. Each was set on one shot and checked on shots it never saw,
against the camera's JPEG with the same look (median CIE76 ΔE over the frame,
30 Oct 2026, GR IIIx 1.70 and GR IV 1.20, GRKit 1.3.3):

| Camera | Gain | Set on | Checked on | ΔE | Lightness vs camera |
| --- | --- | --- | --- | --- | --- |
| GR IIIx | +0.7 EV | native Standard | Portra 400, Ektar 100 portraits | 2.4, 2.6 | −1.4, −1.2 L* |
| GR IV | +0.75 EV | HP5 Plus 400 portrait | Portra 400 (2 scenes), Ektar 100 | 3.6 to 4.4 | −2.2 to −3.8 L* |

The effects are fitted to the GR III, so the GR IV keeps a small tone-curve
difference that a gain does not remove. Previews have no grain.
