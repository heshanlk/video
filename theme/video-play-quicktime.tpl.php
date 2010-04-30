<?php 
//$Id$
/*
 * @file
 * Theme file to handle quicktime output.
 * 
 * Variables passed.
 * $width is the width of the video.
 * $height is the height of the video.
 * $video is the URL of the actual video.
 */
?> 
<object classid="clsid:02BF25D5-8C17-4B23-BC80-D3488ABDDC6B" width="<?php print $width; ?>" height="<?php print $height; ?>" codebase="http://www.apple.com/qtactivex/qtplugin.cab">
  <param name="src" value="<?php print $video; ?>" />
  <param name="autoplay" value="<?php print variable_get('video_autoplay', TRUE); ?>" />
  <param name="kioskmode" value="true" />
  <param name="controller" value="true" />
  <object class="video-object" type="video/quicktime" data="<?php print $video; ?>" width="<?php print $width; ?>" height="<?php print $height; ?>">
    <?php print t('No video?  Get the Quicktime !plugin', array('!plugin' => l(t('Plugin'), 'http://www.apple.com/quicktime/download/'))); ?>
  </object>
</object>