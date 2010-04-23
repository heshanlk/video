<?php
//$Id$
/*
 * @file
 * Class file to handle ffmpeg.
 * 
 * //@todo Add Better commenting.
 */

class video_ffmpeg {
  protected $params = array();
  protected $command = '-y -i !videofile -f flv -ar 22050 -ab !audiobitrate -s !size -b !videobitrate -qscale 1 !convertfile';
  protected $meta_command = '-UP';
  protected $nice = 'nice -n 19';
  protected $flvtool2 = '/usr/bin/flvtool2';

  public function __construct() {
    $this->params['audiobitrate'] = variable_get('video_ffmpeg_helper_auto_cvr_audio_bitrate', 64);
    $this->params['videobitrate'] = variable_get('video_ffmpeg_helper_auto_cvr_video_bitrate', 200);
    $this->params['converter'] = variable_get('video_transcoder_path', '/usr/bin/ffmpeg');
    //this creates a temporary filename inside the tmp directory to write our converted video too
    //@todo maybe just write the converted file to the files directory?
    $this->params['convertfile'] = tempnam(file_directory_temp(), uniqid('video-rendering'));
    $this->params['size'] = variable_get('video_ffmpeg_width', 640) . 'x' . variable_get('video_ffmpeg_height', 480);
    //@todo add this back in after testing.
    //$this->params['command'] = variable_get('video_ffmpeg_helper_auto_cvr_options', $this->command);
    $this->params['command'] = $this->command;
    $this->params['cmd_path'] = variable_get('video_transcoder_path', '/usr/bin/ffmpeg');
    $this->params['metadata_cmd_path'] = variable_get('video_metadata_path', $this->flvtool2);
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
}
?>
