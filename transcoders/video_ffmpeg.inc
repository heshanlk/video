<?php
//$Id$
/*
 * @file
 * Transcoder class file to handle ffmpeg settings and conversions.
 *
 */


class video_ffmpeg implements transcoder_interface {
  // Naming for our radio options.  Makes it easy to extend our transcoders.
  private $name = 'FFmpeg';
  private $value = 'video_ffmpeg';
  private $command;
  
  public function __construct() {
    $this->command = variable_get('video_transcoder_path', '/usr/bin/ffmpeg');
  }

  public function run_command($options) {
    $output = shell_exec($this->command.' '.$options.'  2>&1');
    return $output;
  }

  /**
   * Get some information from the video file
   */
  function get_video_info($video) {
    static $command_ouput;
    if(!empty($command_output)) return $command_output;

    $file = escapeshellarg($video['filepath']);
    // Execute the command
    $command = $this->nice.' '.$this->params['cmd_path'].' -i ' . $file;
    $command_output = shell_exec($command.'  2>&1');
    return $command_output;
  }

  /**
   * Return the playtime seconds of a video
   */
  function get_playtime($video) {
    $ffmpeg_output = $this->get_video_info($video);
    // Get playtime
    $pattern = '/Duration: ([0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9])/';
    preg_match_all($pattern, $ffmpeg_output, $matches, PREG_PATTERN_ORDER);
    $playtime = $matches[1][0];
    // ffmpeg return length as 00:00:31.1 Let's get playtime from that
    $hmsmm = explode(":", $playtime);
    $tmp = explode(".", $hmsmm[2]);
    $seconds = $tmp[0];
    $hours = $hmsmm[0];
    $minutes = $hmsmm[1];
    return $seconds + ($hours * 3600) + ($minutes * 60);
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_name()
   */
  public function get_name() {
    return $this->name;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_value()
   */
  public function get_value() {
    return $this->value;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_help()
   */
  public function get_help() {
    return l(t('FFMPEG Online Manual'), 'http://www.ffmpeg.org/');
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings()
   */
  public function admin_settings() {
    $form = array();
    $form['no_of_video_thumbs'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of thumbnails'),
      '#description' => t('Number of thumbnails to display from video.'),
      '#default_value' => variable_get('no_of_video_thumbs', 5),
    );
    $form['video_ffmpeg_start'] = array(
      '#type' => 'markup',
      '#value' => '<div id="video_ffmpeg">',
    );
    $form['video_transcoder_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to Video Transcoder'),
      '#description' => t('Absolute path to ffmpeg.'),
      '#default_value' => variable_get('video_transcoder_path', '/usr/bin/ffmpeg'),
    );
    $form['video_ffmpeg_end'] = array(
      '#type' => 'markup',
      '#value' => '</div>',
    );

    // Thumbnail Videos We need to put this stuff last.
    $form['autothumb'] = array(
        '#type' => 'fieldset',
        '#title' => t('Video Thumbnails'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE
    );
    $form['autothumb']['video_thumb_path'] = array(
        '#type' => 'textfield',
        '#title' => t('Path to save thumbnails'),
        '#description' => t('Path to save video thumbnails extracted from the videos.'),
        '#default_value' => variable_get('video_thumb_path', 'video_thumbs'),
    );
    $form['autothumb']['advanced'] = array(
        '#type' => 'fieldset',
        '#title' => t('Advanced Settings'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE
    );
    $form['autothumb']['advanced']['video_ffmpeg_thumbnailer_options'] = array(
        '#type' => 'textarea',
        '#title' => t('Video thumbnailer options'),
        '#description' => t('Provide the options for the thumbnailer.  Available argument values are: ').'<ol><li>'.t('!videofile (the video file to thumbnail)').'<li>'.t('!thumbfile (a newly created temporary file to overwrite with the thumbnail)</ol>'),
        '#default_value' => variable_get('video_ffmpeg_thumbnailer_options', '-i !videofile -an -y -f mjpeg -ss !seek -vframes 1 !thumbfile'),
    );

    // Video conversion settings.
    $form['autoconv'] = array(
        '#type' => 'fieldset',
        '#title' => t('Video Conversion'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE
    );
    $form['autoconv']['video_ffmpeg_width'] = array(
        '#type' => 'textfield',
        '#title' => t('Video Width'),
        '#size' => 10,
        '#maxlength' => 10,
        '#default_value' => variable_get('video_ffmpeg_width', 640),
    );
    $form['autoconv']['video_ffmpeg_height'] = array(
        '#type' => 'textfield',
        '#title' => t('Video height'),
        '#size' => 10,
        '#maxlength' => 10,
        '#default_value' => variable_get('video_ffmpeg_height', 480),
    );
    $form['autoconv']['video_ffmpeg_helper_auto_cvr_video_bitrate'] = array(
        '#type' => 'textfield',
        '#title' => t('Video bitrate'),
        '#description' => t('The video bitrate in bit/s of the converted video.'),
        '#size' => 10,
        '#maxlength' => 10,
        '#default_value' => variable_get('video_ffmpeg_helper_auto_cvr_video_bitrate', 200000),
    );
    $form['autoconv']['video_ffmpeg_helper_auto_cvr_audio_bitrate'] = array(
        '#type' => 'textfield',
        '#title' => t('Audio bitrate'),
        '#description' => t('The audio bitrate in bit/s of the converted video.'),
        '#size' => 10,
        '#maxlength' => 10,
        '#default_value' => variable_get('video_ffmpeg_helper_auto_cvr_audio_bitrate', 64000),
    );
    $form['autoconv']['advanced'] = array(
        '#type' => 'fieldset',
        '#title' => t('Advanced Settings'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE
    );
    $form['autoconv']['advanced']['video_ffmpeg_helper_auto_cvr_options'] = array(
        '#type' => 'textarea',
        '#title' => t('Video converter options'),
        '#description' => t('Provide the ffmpeg options to configure the video conversion.  Available argument values are: ').'<ul>'.
        '<li>'.t('!videofile (the video file to convert)').
        '<li>'.t('!convertfile (a newly created file to store the converted file)').
        '<li>'.t('!size (video resolution of the converted file)').
        '</ul>'.t('For further informations refer to the !ffmpegdoc', array('!ffmpegdoc' => l(t('Official FFMpeg documentation.'), 'http://ffmpeg.mplayerhq.hu/ffmpeg-doc.html', array('fragment' => TRUE)))),
        '#default_value' => variable_get('video_ffmpeg_helper_auto_cvr_options', '-y -i !videofile -f flv -ar 22050 -ab !audiobitrate -s !size -b !videobitrate -qscale 1 !convertfile'),
    );
    return $form;
  }
}
?>
