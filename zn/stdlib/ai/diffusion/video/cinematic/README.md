# Cinematic Video Generation

Professional camera controls, lighting presets, and color grading for film-quality videos.

## Quick Start

```nova
from ai.diffusion.video.cinematic import quick_cinematic

video = quick_cinematic(
    prompt="Two cats boxing in spotlight",
    style="epic",
)
video.save("output.mp4")
```

## Features

✅ **13+ Camera Presets** - Hero, drone, thriller, documentary, etc.  
✅ **17+ Lighting Presets** - Golden hour, noir, neon, volumetric  
✅ **11+ Shot Types** - Close-up, wide, medium, extreme wide  
✅ **6 Color Grading** - Cinematic, vintage, bleach bypass  
✅ **8 Style Presets** - Epic, action, horror, commercial  
✅ **One-Click Generation** - Professional results instantly  

## Modules

- `camera.py` - Camera movements, angles, presets
- `shot_types.py` - Cinematic shot framing
- `lighting.py` - Lighting and color grading
- `generator.py` - Main cinematic generator

## Documentation

See [CINEMATIC_VIDEO_GUIDE.md](../../../../docs/CINEMATIC_VIDEO_GUIDE.md) for complete guide.

## Examples

- `examples/cinematic_quick_start.zn` - One-liner
- `examples/cinematic_video_demo.zn` - Full demo
- `examples/cinematic_advanced.zn` - Advanced control
