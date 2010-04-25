<?php
//$Id$
/*
 * @file
 * Transcoder class file to handle ffmpeg settings and conversions.
 *
 */


class video_ffmpeg implements transcoder_interface {
  private $name = 'FFmpeg';
  private $value = 'video_ffmpeg';

  protected $params = array();
  protected $command = '-y -i !videofile -f flv -ar 22050 -ab !audiobitrate -s !size -b !videobitrate -qscale 1 !convertfile';
  protected $thumb_command = '-i !videofile -an -y -f mjpeg -ss !seek -vframes 1 !thumbfile';
  protected $meta_command = '-UP';
  protected $nice = 'nice -n 19';
  protected $ffmpeg = '/usr/bin/ffmpeg';
  protected $flvtool2 = '/usr/bin/flvtool2';

  public function __construct() {
    $this->params['audiobitrate'] = variable_get('video_ffmpeg_helper_auto_cvr_audio_bitrate', 64);
    $this->params['videobitrate'] = variable_get('video_ffmpeg_helper_auto_cvr_video_bitrate', 200);
    //this creates a temporary filename inside the tmp directory to write our converted video too
    //@todo maybe just write the converted file to the files directory?
    $this->params['convertfile'] = tempnam(file_directory_temp(), uniqid('video-rendering'));
    $this->params['size'] = variable_get('video_ffmpeg_width', 640) . 'x' . variable_get('video_ffmpeg_height', 480);
    //@todo add this back in after testing.
    //$this->params['command'] = variable_get('video_ffmpeg_helper_auto_cvr_options', $this->command);
    $this->params['command'] = $this->command;
    
    $this->params['cmd_path'] = variable_get('video_transcoder_path', $this->ffmpeg);
    $this->params['metadata_cmd_path'] = variable_get('video_metadata_path', $this->flvtool2);
    $this->params['thumb_command'] = variable_get('video_ffmpeg_thumbnailer_options', $this->thumb_command);
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#thumbnails($video)
   */
  public function thumbnails($video) {
    global $user;
    // Setup our thmbnail path.
    $video_thumb_path = variable_get('video_thumb_path', 'video_thumbs');
    $final_thumb_path = file_directory_path(). '/' . $video_thumb_path . '/' . $video['fid'];
    // Check our directory first.
    field_file_check_directory($final_thumb_path, FILE_CREATE_DIRECTORY);
    // Total thumbs to generate
    $total_thumbs = variable_get('no_of_video_thumbs', 5);
    $videofile = escapeshellarg($video['filepath']);
    $duration = $this->get_playtime($video);

    $files = NULL;
    for($i = 1; $i <= $total_thumbs; $i++) {
      $seek = ($duration/$total_thumbs) * $i;
      $filename = "/video-thumb-for-$fid-$i.jpg";
      $thumbfile = $final_thumb_path . $filename;
      //skip files already exists, this will save ffmpeg traffic
      if (!is_file($thumbfile)) {        
        $command = $this->nice. ' ' .$this->params['cmd_path']. ' '. t($this->params['thumb_command'], array(
          '!videofile' => $videofile,
          '!seek' => $seek,
          '!thumbfile' => $thumbfile,
        ));
        // Generate the thumbnail from the video.
        $command_output = shell_exec($command.'  2>&1');
        if (!file_exists($thumbfile)) {
          $error_param = array('%file' => $thumbfile, '%cmd' => $command, '%out' => $command_output);
          $error_msg = t("Error generating thumbnail for video: generated file %file does not exist.<br />Command Executed:<br />%cmd<br />Command Output:<br />%out", $error_param);
          // Log the error message.
          watchdog('video_ffmpeg',$error_msg, array(), WATCHDOG_ERROR);
          continue;
        }
      }
      // Begin building the file object.
      // @TODO : use file_munge_filename()
      $file = new stdClass();
      $file->uid      = $user->uid;
      $file->status   = FILE_STATUS_TEMPORARY;
      $file->filename = trim($filename);
      $file->filepath = $thumbfile;
      $file->filemime = file_get_mimetype($filename);
      $file->filesize = filesize($thumbfile);
      $file->timestamp = time();
      $files[] = $file;
    }
    return $files;
  }

  public function convert(&$job) {
    //get current directory for absolute filepath
    $filepath = getcwd() .'/'. $job->filepath;
    $command = t($this->params['command'], array(
      '!videofile' => escapeshellarg($filepath),
      '!audiobitrate' => $this->params['audiobitrate'],
      '!size' => $this->params['size'],
      '!videobitrate' => $this->params['videobitrate'],
      '!convertfile' => $this->params['convertfile']
    ));
    //set the converted file output
    $job->convfile = $this->params['convertfile'];
    //setup our command for execution
    $shell_exec = $this->nice.' '.$this->params['cmd_path'].' '.$command;
    //log our command to the watchdog
    watchdog('video_conversion', 'Executing command: '.$shell_exec, array(), WATCHDOG_DEBUG);
    //Convert the video.
    $command_output = shell_exec($shell_exec.'  2>&1');
    //lets check to make sure our file exists, if not error out
    if(!file_exists($job->convfile) || !filesize($job->convfile)) {
      watchdog('video_conversion', 'Video conversion failed.  FFMPEG reported the following output: '.$command_output, array(), WATCHDOG_ERROR);
    }
    else {
      // If they provide a metadata command path, then we will want to execute that now.
      if (!empty($this->params['metadata_cmd_path'])) {
        $metadata_command = $this->params['metadata_cmd_path']." ".$this->meta_command." '". $this->params['convertfile'] ."'";
        $metadata_command_output = shell_exec($metadata_command.' 2>&1');
        watchdog('video_conversion', 'Executing command: '.$metadata_command, array(), WATCHDOG_DEBUG);
      }
      $file_name = basename($job->filename . ".flv");
      //lets build our files object
      $file = new stdClass();
      $file->uid      = $job->uid;
      $file->status   = FILE_STATUS_PERMANENT;
      $file->filename = basename($file_name);
      $file->filepath = $job->convfile;
      $file->filemime = file_get_mimetype($file_name);
      $file->filesize = filesize($job->convfile);
      $file->timestamp = time();
      //put our file object into the job object
      $job->converted = $file;
    }
  }

  /**
   * Get some information from the video file
   */
  function get_video_info($video) {
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
    $form['tranccoder']['no_of_video_thumbs'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of thumbnails'),
      '#description' => t('Number of thumbnails to display from video.'),
      '#default_value' => variable_get('no_of_video_thumbs', 5),
    );

    // Thumbnail Videos
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
        '#type' => 'textfield',
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
        '#type' => 'textfield',
        '#title' => t('Video converter options'),
        '#description' => t('Provide the ffmpeg options to configure the video conversion.  Available argument values are: ').'<ul>'.
        '<li>'.t('!videofile (the video file to convert)').
        '<li>'.t('!convertfile (a newly created file to store the converted file)').
        '<li>'.t('!size (video resolution of the converted file)').
        '</ul>'.t('For further informations refer to the !ffmpegdoc', array('!ffmpegdoc' => l(t('Official FFMpeg documentation.'), 'http://ffmpeg.mplayerhq.hu/ffmpeg-doc.html', array('fragment' => TRUE)))),
        '#default_value' => variable_get('video_ffmpeg_helper_auto_cvr_options', '-y -i !videofile -f flv -ar 22050 -ab !audiobitrate -s !size -b !videobitrate -qscale 1 !convertfile'),
    );
    $form['video_ffmpeg_end'] = array(
      '#type' => 'markup',
      '#value' => '</div>',
    );
    return $form;
  }
}
?>
