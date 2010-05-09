<?php 
//$Id$
/*
 * @file
 * Theme file to handle quicktime output.
 * 
 * Variables passed.
 * $element is the complete array for the cck field.
 * $width is the width of the video.
 * $height is the height of the video.
 * $video is the URL of the actual video.
 */
?> 
<object classid="clsid:D27CDB6E-AE6D-11cf-96B8-444553540000" width="<?php print $width; ?>" height="<?php print $height; ?>" codebase="http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=6,0,40,0">
  <param name="movie" value="<?php print $video; ?>" />
  <param name="autoplay" value="<?php print variable_get('video_autoplay', TRUE); ?>" />
  <param name="wmode" value="transparent" />
  <object class="video-object" type="application/x-shockwave-flash" data="<?php print $video; ?>" width="<?php print $width; ?>" height="<?php print $height; ?>">
    <?php print t('No video?  Get the Adobe Flash !plugin', array('!plugin' => l(t('Plugin'), 'http://get.adobe.com/flashplayer/'))); ?>
  </object>
</object>