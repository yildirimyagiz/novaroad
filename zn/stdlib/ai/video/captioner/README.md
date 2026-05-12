# VideoCaptioner - AI Video Subtitle Assistant

AI-powered video captioning with speech recognition, LLM optimization, and translation.

## Quick Start

```nova
from ai.video.captioner import quick_caption

result = quick_caption("video.mp4", output_srt="output.srt")
```

## Features

✅ **Speech Recognition** - Whisper local/API  
✅ **LLM Optimization** - Natural sentence flow  
✅ **AI Translation** - Context-aware translation  
✅ **Batch Processing** - Multiple videos  
✅ **Multiple Formats** - SRT, VTT, JSON  

## Modules

- `recognizer.py` - Speech-to-text (Whisper)
- `optimizer.py` - LLM-powered subtitle optimization
- `translator.py` - AI translation with reflection
- `captioner.py` - Main pipeline

## Documentation

See [VIDEO_CAPTIONER_GUIDE.md](../../../../docs/VIDEO_CAPTIONER_GUIDE.md) for complete guide.

## Examples

- `examples/video_captioner_quick.zn` - One-liner
- `examples/video_captioner_demo.zn` - Full demo

## Based On

Original project: [VideoCaptioner](https://github.com/WEIFENG2333/VideoCaptioner) by WEIFENG2333
