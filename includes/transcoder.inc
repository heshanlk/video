<?php
//$Id$
/*
 * @file
 * Class file used to wrap the transcoder functions.
 *
 * @todo need more commenting
 */

class video_transcoder {
  private $transcoder;
  protected $params = array();
  protected $audio_bitrate = 64;
  protected $video_bitrate = 200;
  protected $video_width = 640;
  protected $video_height = 480;
  protected $command = '-y -i !videofile -f flv -ar 22050 -ab !audiobitrate -s !size -b !videobitrate -qscale 1 !convertfile';
  protected $thumb_command = '-i !videofile -an -y -f mjpeg -ss !seek -vframes 1 !thumbfile';
  protected $nice = 'nice -n 19';
  protected $ffmpeg = '/usr/bin/ffmpeg';

  public function __construct() {
    $this->params['audiobitrate'] = variable_get('video_ffmpeg_helper_auto_cvr_audio_bitrate', $this->audio_bitrate);
    $this->params['videobitrate'] = variable_get('video_ffmpeg_helper_auto_cvr_video_bitrate', $this->video_bitrate);
    $this->params['convertfile'] = tempnam(file_directory_temp(), uniqid('video-rendering'));
    $this->params['size'] = variable_get('video_ffmpeg_width', $this->video_width) . 'x' . variable_get('video_ffmpeg_height', $this->video_height);
    $this->params['command'] = variable_get('video_ffmpeg_helper_auto_cvr_options', $this->command);
    $this->params['cmd_path'] = variable_get('video_transcoder_path', $this->ffmpeg);
    $this->params['thumb_command'] = variable_get('video_ffmpeg_thumbnailer_options', $this->thumb_command);

    //get our configured transcoder.
    $transcoder = variable_get('vid_convertor', 'video_ffmpeg');
    module_load_include('inc', 'video', '/transcoders/' . $transcoder);
    if(class_exists($transcoder)) {
      $this->transcoder = new $transcoder;
    }
    else {
      drupal_set_message(t('Transcoder not configured properly'), 'error');
    }
  }

  public function generate_thumbnails($video) {
    global $user;
    // Setup our thmbnail path.
    $video_thumb_path = variable_get('video_thumb_path', 'video_thumbs');
    $final_thumb_path = file_directory_path(). '/' . $video_thumb_path . '/' . $video['fid'];
    // Check our directory first.
    field_file_check_directory($final_thumb_path, FILE_CREATE_DIRECTORY);
    // Total thumbs to generate
    $total_thumbs = variable_get('no_of_video_thumbs', 5);
    $videofile = escapeshellarg($video['filepath']);
    //get the playtime from the current transcoder
    $duration = $this->transcoder->get_playtime($video);

    $files = NULL;
    for($i = 1; $i <= $total_thumbs; $i++) {
      $seek = ($duration/$total_thumbs) * $i;
      $filename = "/video-thumb-for-$fid-$i.jpg";
      $thumbfile = $final_thumb_path . $filename;
      //skip files already exists, this will save ffmpeg traffic
      if (!is_file($thumbfile)) {
        //setup the command to be passed to the transcoder.
        $command = t($this->params['thumb_command'], array('!videofile' => $videofile, '!seek' => $seek, '!thumbfile' => $thumbfile));
        // Generate the thumbnail from the video.
        $command_output = $this->transcoder->run_command($command);
        if (!file_exists($thumbfile)) {
          $error_param = array('%file' => $thumbfile, '%cmd' => $command, '%out' => $command_output);
          $error_msg = t("Error generating thumbnail for video: generated file %file does not exist.<br />Command Executed:<br />%cmd<br />Command Output:<br />%out", $error_param);
          // Log the error message.
          watchdog('video_transcoder',$error_msg, array(), WATCHDOG_ERROR);
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

  public function convert_video(&$job) {
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
    //log our command to the watchdog
    watchdog('video_transcoder', 'Executing command: '.$shell_exec, array(), WATCHDOG_DEBUG);
    //Convert the video with our transcoder.
    $command_output = $this->transcoder->run_command($command);
    //lets check to make sure our file exists, if not error out
    if(!file_exists($job->convfile) || !filesize($job->convfile)) {
      watchdog('video_transcoder', 'Video conversion failed.  FFMPEG reported the following output: '.$command_output, array(), WATCHDOG_ERROR);
    }
    else {
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

    // If they are using metadata.
    if (variable_get('video_metadata', FALSE)) {
      module_load_include('inc', 'video', '/includes/metadata');
      $metadata = new video_metadata;
      $metadata->process($job->convfile);
    }
  }

  public function admin_settings() {
    drupal_add_js(drupal_get_path('module', 'video'). '/js/admin.js');
    $form = array();
    $options = $this->_transcoders();
    $form['vid_convertor'] = array(
      '#type' => 'radios',
      '#title' => t('Video transcoder'),
      '#default_value' => variable_get('vid_convertor', 'video_ffmpeg'),
      '#options' => $options['radios'],
      '#description' => t('Selecting a video transcoder will help you convert videos and generate thumbnails. !list', array('!list' => theme('item_list', $options['help']))),
      '#prefix' => '<div id="transcoder-radios">',
      '#suffix' => '</div>',
    );
    $form = $form + $options['admin_settings'];
    return $form;
  }

  private function _transcoders() {
    // Lets find our transcoder classes and build our radio options
    // We do this by scanning our transcoders folder
    $form = array('radios' => array(), 'help' => array(), 'admin_settings' => array());
    $path = drupal_get_path('module', 'video') .'/transcoders';
    $files = file_scan_directory($path, '^.*\.inc$');
    foreach($files as $file) {
      module_load_include('inc', 'video', '/transcoders/' . $file->name);
      $focus = new $file->name;
      $form['radios'][$focus->get_value()] = $focus->get_name();
      $form['help'][] = $focus->get_help();
      $form['admin_settings'] = $form['admin_settings'] + $focus->admin_settings();
    }
    //we need to move our video/thumbnail fieldsets to the bottom of our form as they are used for each trancoder
    $autothumb = $form['admin_settings']['autothumb'];
    $autoconv = $form['admin_settings']['autoconv'];
    unset($form['admin_settings']['autothumb'], $form['admin_settings']['autoconv']);
    $form['admin_settings']['autothumb'] = $autothumb;
    $form['admin_settings']['autoconv'] = $autoconv;
    return $form;
  }
}

interface transcoder_interface {
  public function run_command($command);
  public function get_playtime($video);
  public function get_name();
  public function get_value();
  public function get_help();
  public function admin_settings();
}