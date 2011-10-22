<?php
/*
 * @file
 * Theme file to handle HTML5 output.
 *
 * Variables passed.
 * $video is the video object (see video_helper->video_object())
 * $node is the Drupal node object
 */

$width = intval($video->player_width);
$height = intval($video->player_height);
$poster = check_plain($video->thumbnail->url);
$autoplayattr = $video->autoplay ? ' autoplay="autoplay"' : '';
$preload = 'none';
if ($video->autobuffering) {
  $preload = 'auto';
}
elseif (variable_get('video_metadata', FALSE)) {
  $preload = 'metadata';
}

$codecs = array(
	'video/mp4' => 'avc1.42E01E, mp4a.40.2',
	'video/webm' => 'vp8, vorbis',
	'video/ogg' => 'theora, vorbis',
	'application/ogg' => 'theora, vorbis',
	'video/ogv' => 'theora, vorbis',
	'video/quicktime' => 'avc1.42E01E, mp4a.40.2',
);

$flashtype = null;
?>
<video width="<?php echo $width; ?>" height="<?php echo $height; ?>" preload="<?php echo $preload; ?>" controls="controls" poster="<?php echo $poster; ?>"<?php echo $autoplayattr; ?>>
<?php
foreach ($video->files as $filetype => $file) {
  $filepath = check_plain($file->url);
  $mimetype = file_get_mimetype($file->filepath);

  if ($mimetype == 'video/quicktime') {
    $mimetype = 'video/mp4';
  }

  if (!isset($codecs[$mimetype])) {
    continue;
  }
  
  // Find the right flash fallback, prefer flv over mp4
  if ($filetype != 'flv' && ($mimetype == 'video/mp4' || $mimetype == 'video/flv')) {
    $flashtype = $filetype;
  }
?>
  <source src="<?php echo $filepath; ?>" type="<?php echo $mimetype; ?>; codecs=&quot;<?php echo $codecs[$mimetype]; ?>&quot;" />
<?php
}

if ($flashtype != null) {
  $video->player = 'flv';
  $video->flash_player = variable_get('video_extension_'. $video->player .'_flash_player', '');

  if ($flashtype != 'flv') {
    $video->files->flv->url = $video->files->{$flashtype}->url;
  }

  echo theme('video_flv', $video, $node);
}
?>
</video>
