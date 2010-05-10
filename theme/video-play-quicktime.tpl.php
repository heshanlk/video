<?php 
//$Id$
/*
 * @file
 * Theme file to handle quicktime output.
 * 
 * Variables passed.
 * $video is the video object.
 * $node is the node object.
 * 
 */
?>
<object classid="clsid:02BF25D5-8C17-4B23-BC80-D3488ABDDC6B" codebase="http://www.apple.com/qtactivex/qtplugin.cab"  width="<?php print $video->width; ?>" height="<?php print $video->height; ?>">
  <param name="src" value="<?php print $video->url; ?>" />
  <param name="controller" value="true" />
  <param name="autoplay" value="<?php print $video->autoplay ? 'true' : 'false'; ?>" />
  <param name="pluginurl" value="http://www.apple.com/quicktime/download/" />
  <embed src="<?php print $video->url; ?>" type="video/quicktime" 
    width="<?php print $video->width; ?>" 
    height="<?php print $video->height; ?>" 
    autostart="<?php print $video->autoplay ? 'true' : 'false'; ?>" 
    controller="true" >
  </embed>
</object>