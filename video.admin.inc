<?php
// $Id$

/**
 * @file
 * Provides the administration settings for the Flowplayer Drupal module.
 */

/**
 * Settings Form
 *
 * @return
 *   string of form content or error message
 */
function video_admin_settings() {
  global $base_url;

  $form = array();

  $form['menu'] = array(
      '#type' => 'fieldset',
      '#title' => t('General Behavior'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE
  );

  //  $form['menu']['video_displaydownloadmenutab'] = array(
  //    '#type' => 'checkbox',
  //    '#title' => t('Display download menu tab'),
  //    '#default_value' => variable_get('video_displaydownloadmenutab', 1),
  //    '#description' => t('Toggle display of menu tab to download video from the node page.')
  //  );
  //  $form['menu']['video_displaydownloadlink'] = array(
  //    '#type' => 'checkbox',
  //    '#title' => t('Display download link'),
  //    '#default_value' => variable_get('video_displaydownloadlink', 1),
  //    '#description' => t('Toggle display of "download" link (below the node content in most themes).')
  //  );
  //  $form['menu']['video_displayplaytime'] = array(
  //      '#type' => 'checkbox',
  //      '#title' => t('Display playtime'),
  //      '#default_value' => variable_get('video_displayplaytime', 1),
  //      '#description' => t('Toggle the display of the playtime for a video.')
  //  );
  //  $form['menu']['video_displayfilesize'] = array(
  //      '#type' => 'checkbox',
  //      '#title' => t('Display filesize'),
  //      '#default_value' => variable_get('video_displayfilesize', 1),
  //      '#description' => t('Toggle the display of the filesize for a video.')
  //  );
  $form['menu']['video_autoplay'] = array(
      '#type' => 'checkbox',
      '#title' => t('Automatically start video on page load'),
      '#default_value' => variable_get('video_autoplay', TRUE),
      '#description' => t('Start the video when the page and video loads')
  );

  $form['menu']['video_autobuffering'] = array(
      '#type' => 'checkbox',
      '#title' => t('Automatically start video buffering'),
      '#default_value' => variable_get('video_autobuffering', TRUE),
      '#description' => t('Start buffering video when the page and video loads')
  );

  $form['player'] = array(
      '#type' => 'fieldset',
      '#title' => t('Video Extra Players'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE
  );
  $form['player']['video_cortado'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to OGG Cortado Player'),
      '#default_value' => variable_get('video_cortado', $base_url . '/cortado.jar'),
      '#description' => t('Copy your cortado.jar file to Drupal root and keep the setting un-changed.')
  );

  $form['player']['mp4_play_in_flowplayer'] = array(
      '#type' => 'checkbox',
      '#title' => t('Play HQ MP4 files in Flash Player'),
      '#default_value' => variable_get('mp4_play_in_flowplayer', FALSE),
      '#description' => t('Play HQ MP4 files in Flash player.')
  );


  //
  $gateways = _video_transcoder_types();
  //  print_r($gateways);
  //  $gateway_options = array();
  //  foreach( $gateways as $gateway) {
  //    $obj = new stdClass();
  //    $obj->option = array($gateway => _smsgateway_invoke( 'name', $gateway));
  //    $gateway_options[] = $obj;
  //  }


  $form['tranccoder'] = array(
      '#type' => 'fieldset',
      '#title' => t('Video Transcoder'),
      '#collapsible' => FALSE,
      '#collapsed' => FALSE
  );

  $form['tranccoder']['vid_convertor'] = array(
      '#type' => 'radios',
      '#title' => t('Video transcoder'),
      '#default_value' => variable_get('vid_convertor', 'ffmpeg'),
      '#options' => $gateways,
      '#description' => t('Video transcoder will help you to video conversion and automatic thumbnail generaion. You must install !ffmpeg_wrapper module to enable ffmpeg_wrapper support', array('!ffmpeg_wrapper' => l('ffmpeg_wrapper module', 'http://drupal.org/project/ffmpeg_wrapper')))
  );
  $form['tranccoder']['video_transcoder_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to Video Transcoder'),
      '#description' => t('Path to executable, you can skip this if your usign ffmpeg_wrapper module support.'),
      '#default_value' => variable_get('video_transcoder_path', '/usr/bin/ffmpeg'),
  );
  $form['tranccoder']['no_of_video_thumbs'] = array(
        '#type' => 'textfield',
        '#title' => t('No of video thumbnails'),
        '#description' => t('No of thumbnails extracting from video'),
        '#default_value' => variable_get('no_of_video_thumbs', 5),
    );

  // ffmpeg wapper settings
  $video_converter = variable_get('vid_convertor', 'ffmpeg');
  if($video_converter == 'ffmpeg') {

    $form['conv'] = array(
        '#type' => 'fieldset',
        '#title' => t('FFmpeg video conversion settings.'),
        '#collapsible' => FALSE,
        '#collapsed' => FALSE
    );
    
    //auto thumb videos
    $form['conv']['autothumb'] = array(
        '#type' => 'fieldset',
        '#title' => t('Automatic video thumbnailing'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE
    );
    $form['conv']['autothumb']['video_thumb_path'] = array(
        '#type' => 'textfield',
        '#title' => t('Path to Video Thumbnails'),
        '#description' => t('Path to save video thumbnails extracted from video'),
        '#default_value' => variable_get('video_thumb_path', 'video_thumbs'),
    );
    $form['conv']['autothumb']['advanced'] = array(
        '#type' => 'fieldset',
        '#title' => t('Advanced settings'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE
    );
    $form['conv']['autothumb']['advanced']['video_ffmpeg_thumbnailer_options'] = array(
        '#type' => 'textfield',
        '#title' => t('Video thumbnailer options'),
        '#description' => t('Provide the options for the thumbnailer.  Available argument values are: ').'<ol><li>'.t('%videofile (the video file to thumbnail)').'<li>'.t('%thumbfile (a newly created temporary file to overwrite with the thumbnail)').'</ol>'.t('Only the first two are mandatory.  For example, older versions of ffmpeg should use something like: !old While newer versions should use something like: !new', array('!old' => "<div>-i %videofile -an -y -f mjpeg -ss %seek -vframes 1 %thumbfile</div>", '!new' => '<div>-i %videofile -an -y -f mjpeg -ss %seek -vframes 1 %thumbfile</div>')),
        '#default_value' => variable_get('video_ffmpeg_thumbnailer_options', '-i %videofile -an -y -f mjpeg -ss %seek -vframes 1 %thumbfile'),
    );


    // automatic video conversion settings
    $form['conv']['autoconv'] = array(
        '#type' => 'fieldset',
        '#title' => t('Automatic video conversion'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE
    );
    $form['conv']['autoconv']['video_ffmpeg_width'] = array(
        '#type' => 'textfield',
        '#title' => t('Destination video Width'),
        '#size' => 10,
        '#maxlength' => 10,
        '#default_value' => variable_get('video_ffmpeg_width', 640),
    );
    $form['conv']['autoconv']['video_ffmpeg_height'] = array(
        '#type' => 'textfield',
        '#title' => t('Destination video height'),
        '#size' => 10,
        '#maxlength' => 10,
        '#default_value' => variable_get('video_ffmpeg_height', 480),
    );
    $form['conv']['autoconv']['video_ffmpeg_helper_auto_cvr_video_bitrate'] = array(
        '#type' => 'textfield',
        '#title' => t('Video bitrate'),
        '#description' => t('The video bitrate in bit/s of the converted video.'),
        '#size' => 10,
        '#maxlength' => 10,
        '#default_value' => variable_get('video_ffmpeg_helper_auto_cvr_video_bitrate', 200000),
    );
    $form['conv']['autoconv']['video_ffmpeg_helper_auto_cvr_audio_bitrate'] = array(
        '#type' => 'textfield',
        '#title' => t('Audio bitrate'),
        '#description' => t('The audio bitrate in bit/s of the converted video.'),
        '#size' => 10,
        '#maxlength' => 10,
        '#default_value' => variable_get('video_ffmpeg_helper_auto_cvr_audio_bitrate', 64000),
    );
    $form['conv']['autoconv']['advanced'] = array(
        '#type' => 'fieldset',
        '#title' => t('Advanced settings'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE
    );
    $form['conv']['autoconv']['advanced']['video_ffmpeg_helper_auto_cvr_options'] = array(
        '#type' => 'textfield',
        '#title' => t('Video converter options'),
        '#description' => t('Provide the ffmpeg options to configure the video conversion.  Available argument values are: ').'<ul>'.
        '<li>'.t('%videofile (the video file to convert)').
        '<li>'.t('%convertfile (a newly created file to store the converted file)').
        '<li>'.t('%size (video resolution of the converted file)').
        '</ul>'.
        t('For further informations refer to the !ffmpegdoc', array('!ffmpegdoc' => l(t('Official FFMpeg documentation.'), 'http://ffmpeg.mplayerhq.hu/ffmpeg-doc.html', array('fragment' => TRUE)))),
        '#default_value' => variable_get('video_ffmpeg_helper_auto_cvr_options', '-y -i %videofile -f flv -ar 22050 -ab %audiobitrate -s %size -b %videobitrate -qscale 1 %convertfile'),
    );

  }
  else if($video_converter == 'ffmpeg_wrapper') {
    global $conf;
    if(module_exists('ffmpeg_wrapper'))
      $form['ffmpeg'] = ffmpeg_wrapper_configuration_form($conf);
  }
  else if($video_converter == 'zencoder') {
      $form['conv']['zencoder'] = array(
          '#type' => 'fieldset',
          '#title' => t('Automatic video conversion'),
          '#collapsible' => TRUE,
          '#collapsed' => FALSE
      );
      $form['conv']['zencoder']['zencoder_key'] = array(
          '#type' => 'textfield',
          '#title' => t('Zencoder Key'),
          '#size' => 10,
          '#maxlength' => 10,
          '#default_value' => variable_get('zencoder_key', ''),
      );
      $form['conv']['zencoder']['s3_key'] = array(
          '#type' => 'textfield',
          '#title' => t('Destination video height'),
          '#size' => 10,
          '#maxlength' => 10,
          '#default_value' => variable_get('s3_key', ''),
      );
    }

  return system_settings_form($form);
}

/**
 * Form API callback to validate the upload settings form.
 *
 * Keeps the use from showing the play tab or the play link
 * if they have chosen to display the video in the node body.
 *
 *
 */
function video_admin_settings_validate($form, &$form_state) {
// check ffmpeg_wrapper installed or not
  if ($form_state['values']['vid_convertor'] == 'ffmpeg_wrapper' && !module_exists('ffmpeg_wrapper')) {
    form_set_error('vid_convertor', t('You do not have installed the !ffmpeg_wrapper module to enable using its plugin, please install it.' , array('!ffmpeg_wrapper' => l('ffmpeg_wrapper', 'http://drupal.org/project/ffmpeg_wrapper'))));
  }
}

/**
 * Ensure that specified convertor is included.
 */
function _video_transcoder( $gateway) {
  $path = drupal_get_path('module', 'video') . '/plugins';
  $files = file_scan_directory($path, '^'. $gateway .'\.inc$');
  foreach ($files as $filename => $file) {
    include_once($filename);
  }
}

/**
 *
 * @staticvar <type> $transcoders
 * @return <type> get installed types of video
 */
function _video_transcoder_types() {
  static $transcoders;

  if (!isset($transcoders)) {
    $transcoders = array();
    $path = drupal_get_path('module', 'video') .'/plugins';
    $files = file_scan_directory($path, '^.*\.inc$');
    foreach ($files as $filename => $file) {
      include_once($filename);
      $transcoders[$file->name] = $file->name;
    }
  }
  return $transcoders;
}

?>
