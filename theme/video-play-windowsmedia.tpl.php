<?php 
//$Id$
/*
 * @file
 * Theme file to handle windows media output.
 * 
 * Variables passed.
 * $element is the complete array for the cck field.
 * $width is the width of the video.
 * $height is the height of the video.
 * $video is the URL of the actual video.
 */
?>

<object type="video/x-ms-wmv" data="<?php print $video; ?>" width="<?php print $width; ?>" height="<?php print $height; ?>">
    <param name="src" value="<?php print $video; ?>" valuetype="ref" type="<?php print $video; ?>">
    <param name="animationatStart" value="true">
    <param name="transparentatStart" value="true">
    <param name="autostart" value="<?php print variable_get('video_autoplay', TRUE); ?>">
    <param name="controller" value="1">
    <?php print t('No video?  Get the Windows Media !plugin', array('!plugin' => l(t('Plugin'), 'http://www.microsoft.com/windows/windowsmedia/player/download/'))); ?>
</object>