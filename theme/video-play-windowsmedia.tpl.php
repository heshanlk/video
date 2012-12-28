<?php
/**
 * @file
 * Theme file to handle windows media output.
 *
 * Variables passed.
 * $video is the video object.
 * $node is the node object.
 */

// handle missing/disabled Windows Media Player plugins
$bua = $_SERVER['HTTP_USER_AGENT'];
if(strpos($bua, 'MSIE')) {
  $fallbacklink = l(t('Windows Media Plugin'), url('http://www.sevenforums.com/media-center/161359-media-player-wont-play-web-wmv-files-anymore-5.html'));
}
elseif(strpos($bua, 'Firefox')) {
  $fallbacklink = l(t('Windows Media Plugin'), url('http://www.interoperabilitybridges.com/windows-media-player-firefox-plugin-download'));
}
elseif(strpos($bua, 'Chrome')) {
  $fallbacklink = l(t('Windows Media Plugin'), url('http://www.interoperabilitybridges.com/wmp-extension-for-chrome'));
}
else {
  $fallbacklink = l(t('Windows Media Player'), url('http://windows.microsoft.com/en-us/windows/windows-media-player'));
}
?>
<object type="video/x-ms-wmv" data="<?php print $video->files->{$video->player}->url; ?>" width="<?php print $video->player_width; ?>" height="<?php print $video->player_height; ?>">
  <param name="src" value="<?php print $video->files->{$video->player}->url; ?>" valuetype="ref" type="<?php print $video->files->{$video->player}->url; ?>">
  <param name="animationatStart" value="true">
  <param name="transparentatStart" value="true">
  <param name="autostart" value="<?php print $video->autoplay; ?>">
  <param name="controller" value="1">
  <p><?php print t('No video? Check that your WMV plugin is enabled in browser options, or get the !pluginlink', array('!pluginlink' => $fallbacklink)); ?></p>
</object>