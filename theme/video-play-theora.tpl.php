<?php 
//$Id$
/*
 * @file
 * Theme file to handle ogg theora output.
 * 
 * Variables passed.
 * $width is the width of the video.
 * $height is the height of the video.
 * $video is the URL of the actual video.
 */
?> 
<applet=code="com.fluendo.player.Cortado.class" archive="<?php print variable_get('video_ogg_player', 'http://theora.org/cortado.jar'); ?>" width="<?php print $width; ?>" height="<?php print $height; ?>">
  <param name="url" value="<?php print $video; ?>" />
  <param name="local" value="false" />
  <param name="mode" value="zero" />
  <param name="keepaspect" value="true" />
  <param name="video" value="true" />
  <param name="audio" value="true" />
  <param name="seekable" value="true" />
  <param name="bufferSize" value="200" />
  <?php print t('No video?  Get the Latest Cortado !plugin', array('!plugin' => l(t('Plugin'), 'http://www.theora.org/cortado/'))); ?>
</applet>