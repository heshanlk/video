<?php 
//$Id$
/*
 * @file
 * Theme file to handle windows media output.
 * 
 * Variables passed.
 * $width is the width of the video.
 * $height is the height of the video.
 * $video is the URL of the actual video.
 */
?> 
<object classid="clsid:6BF52A52-394A-11d3-B153-00C04F79FAA6" width="<?php print $width; ?>" height="<?php print $height; ?>" type="application/x-oleobject">
  <param name="src" value="<?php print $video; ?>" />
  <param name="url" value="<?php print $video; ?>" />
  <param name="autostart" value="<?php print variable_get('video_autoplay', TRUE); ?>" />
  <object class="video-object" type="application/x-mplayer2" data="<?php print $video; ?>" width="<?php print $width; ?>" height="<?php print $height; ?>">
    <?php print t('No video?  Get the Windows Media !plugin', array('!plugin' => l(t('Plugin'), 'http://www.microsoft.com/windows/windowsmedia/player/download/'))); ?>
  </object>
</object>