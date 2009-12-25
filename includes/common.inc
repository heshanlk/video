<?php
//$Id$
/**
 * @file
 * Add some common functions for the various video types supported
 *
 * @todo   Configure default players by video filetype (user configurable?)
 */


/**
 * Get the object for the suitable player for the parameter resource
 */
function _video_common_get_player($element) {
  $field = content_fields($element['#field_name'], $element['#type_name']);
  $resolution = array_filter(explode(':', $field['widget']['default_resolution']));
  $element['#item']['data']['height'] = $element['#item']['data']['width'] * ($resolution[1]/$resolution[0]);

  $op = _video_get_filetype($element['#item']['filename']);
  switch ($op) {
    case 'divx':
      return theme('video_play_divx', $element);
    case 'mov':
    case '3gp':
    case '3g2':
    case 'mp4': // video/mp4
      return theme('video_play_quicktime', $element);
    case 'rm':
      return theme('video_play_realmedia', $element);
    case 'f4v':
    case 'flv': // flowplayer also supprts MP4, H.264 (.extension?)
      return theme('video_play_flash', $element);
    case 'swf':
      return theme('video_play_swf', $element);
    case 'dir':
    case 'dcr':
      return theme('video_play_dcr', $element);
    case 'asf':
    case 'wmv':
    case 'avi':
    case 'mpg':
    case 'mpeg':
      return theme('video_play_windowsmedia', $element);
    case 'ogg':
      return theme('video_play_ogg_theora', $element);
    default:
      drupal_set_message('No video player is configured for ' .$op, 'error');
      break;
  }
}


/**
 * Pull the file extension from a filename
 *
 * @param $vidfile
 *   string filename to get the filetype from.
 *
 * @return
 *   string value of file type or boolean FALSE on error
 */
function _video_get_filetype($vidfile) {
  if (strstr($vidfile, '.')) { //If file contains a "." then get the file extension after the ".
    $file_type = end(explode('.', $vidfile));
  }
  else {
    $file_type = FALSE;
  }

  return strtolower($file_type);
}

/*********************************************************************
 * Themeable functions for playing videos. They print a page with a player embedded.
 *********************************************************************/

/**
 * Play videos from in FLV Flash video format
 *
 * @param $node
 *   object with node information
 *
 * @return
 *   string of content to display
 */
function theme_video_play_flash($element) {
  //TODO : remove item height set in here
  $video = file_create_url($element['#item']['filepath']);
//  echo file_create_url($element['#item']['filepath']);
  $width = isset($element['#item']['data']['width']) ? $element['#item']['data']['width'] : '';
  $height = isset($element['#item']['data']['height']) ? $element['#item']['data']['height'] : '';
  $width = empty($width) ? '350px' : $width .'px';
  $height = empty($height) ? '285px' : $height .'px';
  $id = $element['#formatter'];

  if (module_exists('flowplayer')) {
    $output = theme('flowplayer', array(
        'clip' => array(
        'url' =>  $video,
        //TODO: Make settings for this
        'autoPlay' => FALSE, // Turn autoplay off
        'autoBuffering' => TRUE,
        ),
        ),
        $id, array(
        'style' => "width: $width; height: $height",
    )
  );
  }
  else {
    $output='<p>Use <b>SwfTools Module</b> to play FLV files if you only convert/upload FLV videos or <b>FlowPlayer Module</b> must be enabled in order to play FLV videos with other types.</p>';
  }
  return $output;
}

/**
 * Play Flash .swf files.
 *
 * @param $node
 *   object with node information
 *
 * @return
 *   string of content to display
 */
function theme_video_play_swf($element) {

  $video = file_create_url($element['#item']['filepath']);
  $width = isset($element['#item']['data']['width']) ? $element['#item']['data']['width'] : '';
  $height = isset($element['#item']['data']['height']) ? $element['#item']['data']['height'] : '';
  $width = empty($width) ? '350px' : $width .'px';
  $height = empty($height) ? '285px' : $height .'px';


  // this will be executed by not Internet Explorer browsers
  $output = '<!--[if !IE]> <-->
<object type="application/x-shockwave-flash" width="'. $width .'" height="'. $height .'"
data="'. $video .'">
<!--> <![endif]-->' . "\n";

  // this will be executed by Internet Explorer
  $output .= '<!--[if IE]>
<object type="application/x-shockwave-flash" width="'. $width .'" height="'. $height .'"
classid="clsid:D27CDB6E-AE6D-11cf-96B8-444553540000"
codebase="http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=6,0,40,0">
<![endif]-->' . "\n";

  // params will be passed to both IE or not IE browsers
  $output .= '<param name="movie" value="'. $video .'" />'. "\n" .
      '<param name="wmode" value="transparent" />' . "\n"
      . //_video_get_parameters($node) .
      '<p>'. t('Your browser is not able to display this multimedia content.') .'</p>
</object>';
/*
  $output = theme('video_format_play', $output, t('http://www.macromedia.com/go/getflashplayer'), t('Link to Flash player download'), t('Download the latest Flash player'));*/
  return $output;
}



/**
 * Play Director .dcr/.dir files.
 *
 * @param $node
 *   object with node information
 *
 * @return
 *   string of content to display
 */

function theme_video_play_dcr($element) {

  $video = file_create_url($element['#item']['filepath']);
  $width = isset($element['#item']['data']['width']) ? $element['#item']['data']['width'] : '';
  $height = isset($element['#item']['data']['height']) ? $element['#item']['data']['height'] : '';
  $width = empty($width) ? '350px' : $width .'px';
  $height = empty($height) ? '285px' : $height .'px';

  // this will be executed by not Internet Explorer browsers
  $output = '<!--[if !IE]> <-->
<object type="application/x-director" width="'. $width .'" height="'. $height .'"
data="'. $video .'">
<!--> <![endif]-->' . "\n";

  // this will be executed by Internet Explorer
  $output .= '<!--[if IE]>
<object type="application/x-director" width="'. $width .'" height="'. $height .'"
classid="clsid:166B1BCA-3F9C-11CF-8075-444553540000"
codebase="http://download.macromedia.com/pub/shockwave/cabs/director/sw.cab#version=10,0,0,0">
<![endif]-->' . "\n";

  // params will be passed to both IE or not IE browsers
  $output .= '<param name="src" value="'. $video .'" />' . "\n"
      . //_video_get_parameters($node) .
      '<p>'. t('Your browser is not able to display this multimedia content.') .'</p>
</object>';
/*
  $output = theme('video_format_play', $output, t('http://www.macromedia.com/shockwave/download/'),
                                      t('Link to Macromedia Shockwave Player Download Page'),
                                      t('Download latest Shockwave Player'));*/
  return $output;
}

/**
 * Play videos from in DivX format
 *
 * @see http://developer.apple.com/internet/ieembedprep.html
 * @param $node
 *   object with node information
 *
 * @return
 *   string of content to display
 */
function theme_video_play_divx($element) {
//Increase the height to accommodate the player controls on the bottom.
  $video = file_create_url($element['#item']['filepath']);
  $width = isset($element['#item']['data']['width']) ? $element['#item']['data']['width'] : '';
  $height = isset($element['#item']['data']['height']) ? $element['#item']['data']['height'] : '';
  $width = empty($width) ? '350px' : $width .'px';
  $height = empty($height) ? '285px' : $height .'px';

  //$url = _video_get_fileurl($node->vidfile);

  $output = '<!-- [if IE] -->
<object classid="clsid:67DABFBF-D0AB-41fa-9C46-CC0F21721616" width="'.$width.'" height="'.$height.'" codebase="http://go.divx.com/plugin/DivXBrowserPlugin.cab">
<!--> <![endif]-->'. "\n";
  // this will be executed by not Internet Explorer browsers
  $output = '<!-- [if !IE] -->
<object type="video/divx" data="'.$video.'" width="'.$width.'" height="'.$height.'" mode="zero">
<!--> <![endif]-->'."\n";

  $output .= '<param name="src" value="'.$video.'"/>'."\n";
  $output .= '<param name="mode" value="zero"/>'."\n";
  $output .= '</object>';
  /*
  $output = theme('video_format_play', $output,t('http://www.divx.com/divx/webplayer/'),
                                     t('Link to DivX Download Page'),
                                     t('Download latest DivX Web Player'));*/
  return $output;
}

/**
 * Play videos from in Quicktime format
 *
 * @see http://developer.apple.com/internet/ieembedprep.html
 * @param $node
 *   object with node information
 *
 * @return
 *   string of content to display
 */
function theme_video_play_quicktime($element) {
//Increase the height to accommodate the player controls on the bottom.
  $video = file_create_url($element['#item']['filepath']);
  $width = isset($element['#item']['data']['width']) ? $element['#item']['data']['width'] : '';
  $height = isset($element['#item']['data']['height']) ? $element['#item']['data']['height'] : '';
  $width = empty($width) ? '350px' : $width .'px';
  $height = empty($height) ? '285px' : $height .'px';


  // this will be executed by not Internet Explorer browsers
  $output = '<!--[if !IE]> <-->
<object type="video/quicktime" width="'. $width .'" height="'. $height .'"
data="'. $video .'">
<!--> <![endif]-->' . "\n";

  // this will be executed by Internet Explorer
  $output .= '<!--[if IE]>
<object classid="clsid:02BF25D5-8C17-4B23-BC80-D3488ABDDC6B" codebase="http://www.apple.com/qtactivex/qtplugin.cab" width="'. $width .'" height="'. $height .'" scale="tofit" >
<![endif]-->' . "\n";

  // params will be passed to both IE or not IE browsers
  //GMM: kioskmode enabled so users don't bypass download security video through player
  $output .= '<param name="src" value="'. $video .'" />
              <param name="AUTOPLAY" value="'.(variable_get('video_autoplay', TRUE) ? TRUE : FALSE).'" />
              <param name="KIOSKMODE" value="true" />
              <param name="CONTROLLER" value="true" />' . "\n"
      . //_video_get_parameters($node) .
      '<p>'. t('Your browser is not able to display this multimedia content.') .'</p>
</object>'; // only one </object> needed becouse only one opening tag has been parsed by browsers


  /*
  $output = '<script language="JavaScript" type="text/javascript">';
  $output .= "InsertQuicktimeVideo('{$node->vidfile}','$height','{$node->video_scaled_x}');";
  $output .= '</script>';
  */

  //Fix for bug http://drupal.org/node/630606
/*
  $output = theme('video_format_play', $output, t('http://www.apple.com/quicktime/download'),
                                      t('Link to QuickTime Download Page'),
                                      t('Download latest Quicktime Player'));
									  */
  return $output;
}

/**
 * Play videos from in Realmedia format
 *
 * @param $node
 *   object with node information
 *
 * @return
 *   string of content to display
 */
function theme_video_play_realmedia($element) {
// Real's embeded player includes the controls
// in the height
  $video = file_create_url($element['#item']['filepath']);
  $width = isset($element['#item']['data']['width']) ? $element['#item']['data']['width'] : '';
  $height = isset($element['#item']['data']['height']) ? $element['#item']['data']['height'] : '';
  $width = empty($width) ? '350px' : $width .'px';
  $height = empty($height) ? '285px' : $height .'px';

  // this will be executed by not Internet Explorer browsers
  $output = '<!--[if !IE]> <-->
<object type="audio/x-pn-realaudio-plugin" width="'. $width .'" height="'. $height .'"
data="'. $video .'">
<!--> <![endif]-->' . "\n";

  // this will be executed by Internet Explorer
  $output .= '<!--[if IE]>
<object type="audio/x-pn-realaudio-plugin" width="'. $width .'" height="'. $height .'"
classid="clsid:CFCDAA03-8BE4-11cf-B84B-0020AFBBCCFA" >
<![endif]-->' . "\n";

  // params will be passed to both IE or not IE browsers
  $output .= '<param name="src" value="'. $video .'" />
              <param name="_ExtentX" value="7276" />
              <param name="" value="3307" />
              <param name="AUTOSTART" value="'.(variable_get('video_autoplay', TRUE) ? 'true' : 'false').'" />
              <param name="SHUFFLE" value="0" />
              <param name="PREFETCH" value="0" />
              <param name="NOLABELS" value="0" />
              <param name="CONTROLS" value="All" />
              <param name="CONSOLE" value="Clip1" />
              <param name="LOOP" value="0" />
              <param name="NUMLOOP" value="0" />
              <param name="CENTER" value="0" />
              <param name="MAINTAINASPECT" value="1" />
              <param name="BACKGROUNDCOLOR" value="#000000" />'
      . //_video_get_parameters($node) .
      '<p>'. t('Your browser is not able to display this multimedia content.') .'</p>
</object>'; // only one </object> needed becouse only one opening tag has been parsed by browsers

/*
  $output = theme('video_format_play', $output, t('http://www.real.com/'),
                                      t('Link to Real'),
                                      t('Download latest Realmedia Player'));*/
  return $output;
}

/**
 * Play videos from in WindowsMediaVideo format
 *
 * @param $node
 *   object with node information
 *
 * @return
 *   string of content to display
 */
function theme_video_play_windowsmedia($element) {
// Windows Media's embeded player includes the controls in the height
  $video = file_create_url($element['#item']['filepath']);
  $width = isset($element['#item']['data']['width']) ? $element['#item']['data']['width'] : '';
  $height = isset($element['#item']['data']['height']) ? $element['#item']['data']['height'] : '';
  $width = empty($width) ? '350px' : $width .'px';
  $height = empty($height) ? '285px' : $height .'px';

  // this will be executed by not Internet Explorer browsers
  $output = '<!--[if !IE]> <-->
<object type="application/x-mplayer2" width="'. $width .'" height="'. $height .'"
data="'. $video .'">
<!--> <![endif]-->' . "\n";

  // this will be executed by Internet Explorer
  $output .= '<!--[if IE]>
<object type="application/x-oleobject" width="'. $width .'" height="'. $height .'"
classid="clsid:6BF52A52-394A-11d3-B153-00C04F79FAA6" >
<![endif]-->' . "\n";

  // params will be passed to both IE or not IE browsers
  $output .= '<param name="src" value="'. $video .'" />
              <param name="URL" value="'.$video.'" />
              <param name="animationatStart" value="true" />
              <param name="transparentatStart" value="true" />
              <param name="autoStart" value="'.(variable_get('video_autoplay', TRUE) ? 'true' : 'false').'" />
              <param name="showControls" value="true" />
              <param name="loop" value="true" />'
      . //_video_get_parameters($node) .
      '<p>'. t('Your browser is not able to display this multimedia content.') .'</p>
</object>'; // only one </object> needed becouse only one opening tag has been parsed by browsers


  $output = "\n<div id=\"video-player\">\n" . $output;
  $output .= "<p>\n". t('Problems viewing videos?');
  $output .= "<br />\n";
  $output .= l(t('Download latest Windows Media Player'), t('http://windowsupdate.microsoft.com/'), array('attributes' => array('title' => t('Link to Windows Update')), 'absolute' => TRUE));
  $output .= "<br />\n";
  $output .= l(t('Download the plug-in for Firefox'), t('http://support.mozilla.com/en-US/kb/Using+the+Windows+Media+Player+plugin+with+Firefox'), array('attributes' => array('title' => t('Link to Firefox Plug-in')), 'absolute' => TRUE));
  $output .= "\n</p> \n </div>\n";
  return $output;
}




/**
 * Play Ogg Theora videos with Cortado Applet
 *
 * @param $node
 *   object with node information
 *
 * @return
 *   string of content to display
 */
function theme_video_play_ogg_theora($element) {
  global $base_url;
  $cortado_location = variable_get('video_cortado', $base_url . '/cortado.jar');
  //$url = _video_get_fileurl($node->vidfile);
  $video = file_create_url($element['#item']['filepath']);

  $width = ($node->video_scaled_x ? $node->video_scaled_x : '425');
  $height = ($node->video_scaled_y ? $node->video_scaled_y : '350');

  $output = '
  <!--[if !IE]>-->
  <object classid="java:com.fluendo.player.Cortado.class"
          type="application/x-java-applet"
          archive="' . $cortado_location . '"
          width="' . $width . '" height="' . $height . '" >
  <!--<![endif]-->
    <object classid="clsid:8AD9C840-044E-11D1-B3E9-00805F499D93"
              codebase="http://java.sun.com/update/1.5.0/jinstall-1_5_0-windows-i586.cab"
              width="' . $width . '" height="' . $height . '" >
        <param name="code" value="com.fluendo.player.Cortado" />
    <!--[if !IE]>-->
    </object>
    <!--<![endif]-->
      <!-- IE and Konqueror browser need the archive param -->
      <param name="archive" value="' . $cortado_location . '" />
      <param name="url" value="' . $video . '"/>
      <param name="local" value="false" />
      <param name="keepaspect" value="true" />
      <param name="video" value="true" />
      <param name="audio" value="true" />
      <param name="seekable" value="true" />
      <param name="duration" value="' . $node->playtime_seconds . '" />
      <param name="bufferSize" value="200" />
      <strong>
          This browser does not have a Java Plug-in.<br />
          <a href="http://java.com/download/">
            Get the latest Java Plug-in here.
          </a>
      </strong>
  </object>
  ';
/*
  $output = theme('video_format_play', $output,
    t('http://java.com/download/'), t('Link to java.com'), t('Download Java'));*/
  return $output;
}