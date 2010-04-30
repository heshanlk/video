<?php 
//$Id$
/*
 * @file
 * Theme file to handle director output.
 * 
 * Variables passed.
 * $width is the width of the video.
 * $height is the height of the video.
 * $video is the URL of the actual video.
 */
?> 
<object classid="clsid:166B1BCA-3F9C-11CF-8075-444553540000" type="application/x-director" width="<?php print $width; ?>" height="<?php print $height; ?>" codebase="http://download.macromedia.com/pub/shockwave/cabs/director/sw.cab#version=10,0,0,0">
  <param name="src" value="<?php print $video; ?>" />
  <object class="video-object" type="application/x-director" data="<?php print $video; ?>" width="<?php print $width; ?>" height="<?php print $height; ?>" mode="zero">
    <?php print t('No video?  Get the Director !plugin', array('!plugin' => l(t('Plugin'), 'http://www.macromedia.com/shockwave/download/'))); ?>
  </object>
</object>