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
<object classid="clsid:02BF25D5-8C17-4B23-BC80-D3488ABDDC6B" codebase="http://www.apple.com/qtactivex/qtplugin.cab"  width="<?php print $width; ?>" height="<?php print $height; ?>">
  <param name="src" value="<?php print $video; ?>" />
  <param name="controller" value="true" />
  <param name="autoplay" value="<?php print variable_get('video_autoplay', TRUE); ?>" />
  <param name="pluginurl" value="http://www.apple.com/quicktime/download/" />
  <embed src="<?php print $video; ?>" type="video/quicktime" 
    width="<?php print $width; ?>" 
    height="<?php print $height; ?>" 
    autostart="<?php print variable_get('video_autoplay', TRUE); ?>" 
    controller="true" >
  </embed>
</object>