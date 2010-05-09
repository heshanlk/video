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
<object classid="clsid:CFCDAA03-8BE4-11cf-B84B-0020AFBBCCFA" width="<?php print $width; ?>" height="<?php print $height; ?>">
  <param name="src" value="<?php print $video; ?>" />
  <param name="autostart" value="<?php print variable_get('video_autoplay', TRUE); ?>" />
  <param name="controls" value="imagewindow" />
  <param name="console" value="video" />
  <param name="loop" value="false" />
  <object class="video-object" type="audio/x-pn-realaudio-plugin" data="<?php print $video; ?>" width="<?php print $width; ?>" height="<?php print $height; ?>">
    <?php print t('No video?  Get the Real Media !plugin', array('!plugin' => l(t('Plugin'), 'http://www.real.com/realplayer'))); ?>
  </object>
</object>