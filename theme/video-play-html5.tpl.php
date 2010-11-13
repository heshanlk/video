<?php
//$Id$
/*
 * @file
 * Theme file to handle HTML5 output.
 *
 * Variables passed.
 * $video is the video object.
 * $node is the node object.
 *
 *
 */
?>

<video width="<?= $video->player_width; ?>" height="<?= $video->player_height; ?>" controls>
  <!-- MP4 must be first for iPad! -->
  <source src="<?= $video->url; ?>" type="video/mp4" /><!-- WebKit video    -->
  <source src="<?= $video->url; ?>" type="video/ogg" /><!-- Firefox / Opera -->
  <!-- fallback to Flash: -->
</video>
