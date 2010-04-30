<?php 
//$Id$
/*
 * @file
 * Theme file to handle divx output.
 * 
 * Variables passed.
 * $width is the width of the video.
 * $height is the height of the video.
 * $video is the URL of the actual video.
 */
?> 
<object classid="clsid:67DABFBF-D0AB-41fa-9C46-CC0F21721616" width="<?php print $width; ?>" height="<?php print $height; ?>" codebase="http://go.divx.com/plugin/DivXBrowserPlugin.cab">
  <param name="src" value="<?php print $video; ?>" />
  <param name="pluginspage" value="http://go.divx.com/plugin/download/" />
  <param name="mode" value="zero" />
  <object class="video-object" type="video/divx" data="<?php print $video; ?>" width="<?php print $width; ?>" height="<?php print $height; ?>" mode="zero">
    <?php print t('No video?  Get the DivX Web Player !plugin', array('!plugin' => l(t('Plugin'), 'http://go.divx.com/plugin/download/'))); ?>
  </object>
</object>