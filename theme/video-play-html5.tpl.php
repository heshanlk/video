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

<video width="<?= $video->player_width; ?>" height="<?= $video->player_height; ?>" controls poster="<?php print $video->thumbnail->url; ?>">
  <!-- MP4 must be first for iPad! -->
  <?php foreach ($video->files as $filetype => $values) : ?>
    <source src="<?php echo $values['url']; ?>" type="<?php echo file_get_mimetype($values['filepath']) ?>" /><!-- WebKit video    -->
   <?php endforeach; ?>
  <!-- fallback to Flash: -->
</video>
