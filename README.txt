# Video module 2 for Drupal 7

This readme file is still under construction.

## Troubleshooting

### FFmpeg errors

#### "File for preset 'xyz' not found"

Select "None" in the "FFmpeg video preset" drop down for your preset.

#### "broken ffmpeg default settings detected" "use an encoding preset (vpre)"

Select "libx264-default" in the "FFmpeg video preset" drop down for your preset.

#### "Could not write header for output file #0"

You probably selected the wrong codec for your extension. For instance,
for MP4 you need to select the libx264 video codec and AAC audio codec.


