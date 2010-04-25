<?php
//$Id$
/*
 * @file
 * Class file to handle video conversion using ffmpeg.
 * 
 */

//@todo: Lets move this define to a variable and allow the user to set it through the UI.
define('VIDEO_RENDERING_FFMPEG_INSTANCES', 5);

define('VIDEO_RENDERING_PENDING', 1);
define('VIDEO_RENDERING_ACTIVE', 5);
define('VIDEO_RENDERING_COMPLETE', 10);
define('VIDEO_RENDERING_FAILED', 20);

class video_conversion {
  
  /**
   * Our main function to call when converting queued up jobs.
   */
  public function run_queue() {
    if ($jobs = $this->select_queue()) {
      foreach ($jobs as $job) {
        $this->process($job);
      }
    }
    else {
      watchdog('video_conversion', 'No video conversion jobs to schedule.', array(), WATCHDOG_DEBUG);
    }
  }

  /**
   * Select VIDEO_RENDERING_FFMPEG_INSTANCES videos from queue
   * 
   * @return
   *   An array containing all the videos to be proccessed.
   */
  private function select_queue() {
    $jobs = array();
    $result = db_query_range('SELECT f.* FROM {video_rendering} vr LEFT JOIN {files} f ON vr.fid = f.fid WHERE vr.status = %d AND f.status = %d ORDER BY f.timestamp',
    VIDEO_RENDERING_PENDING, FILE_STATUS_PERMANENT, 0, VIDEO_RENDERING_FFMPEG_INSTANCES);

    while ($row = db_fetch_object($result)) {
      $jobs[] = $row;
    }
    return $jobs;
  }

  /**
   * Process the video through ffmpeg.
   * 
   * @param $video
   *   This can either be the file object or the file id (fid)
   *   
   * @return
   *   TRUE of FALSE if video was converted successfully.
   */
  public function process($video) {
    if (is_object($video) && isset($video->fid)) {
      $return = $this->render($video);
    }
    else {
      $video_object = $this->load_video($video);
      $return = $this->render($video_object);
    }
    return $return;
  }

  private function render($job) {
    if (!is_object($job)) {
      watchdog('video_conversion', 'Job object is not present', array(), WATCHDOG_ERROR);
      return FALSE;
    }
    
    // This will update our current job status to active.
    $this->change_status($job->fid, VIDEO_RENDERING_ACTIVE);
    // Get the converted file object
    if ($this->convert_file($job)) {
      if (!isset($job->converted) || !is_object($job->converted)) {
        watchdog('video_conversion', 'Converted file is an empty file.', array(), WATCHDOG_ERROR);
        $this->change_status($job->fid, VIDEO_RENDERING_FAILED);
        return FALSE;
      }
      // Get our converted file object out of the video object.
      $file = $job->converted;
      // Setup our destination directory for the new file to be copied too.
      $dest_dir = dirname($job->filepath).'/';
      // Update the file table entry and copy the converted file to the files directory.
      if (file_copy($file, $dest_dir)) {
        $file->fid = $job->fid;
        drupal_write_record('files', $file, 'fid');
        // Add new entry for old video
        drupal_write_record('files', $job);
        // Update our status to completed for the original file id we overwrote with the converted video
        $this->change_status($file->fid, VIDEO_RENDERING_COMPLETE);
        // Why are we clearing cache?
        cache_clear_all('*', 'cache_content', true);
        watchdog('video_conversion', 'Successfully converted %orig to %dest', array('%orig' => $job->filepath, '%dest' => $file->filepath), WATCHDOG_INFO);
        return TRUE;
      }
      else {
        $this->change_status($job->fid, VIDEO_RENDERING_FAILED);
        // Get the username of the process owner.
        $ownerarray = posix_getpwuid(posix_getuid());
        $owner = $ownerarray['name'];
        // Get the username of the destination folder owner.
        $fownerarray = posix_getpwuid(fileowner($dest_dir));
        $fowner = $fownerarray['name'];
        // Get destination folder permissions.
        $perms = substr(sprintf('%o', fileperms($dest_dir)), -4);
        watchdog('video_conversion', 'Error moving video %vid_file with nid = %nid to %dir the final directory. Check folder permissions.<br />The script was run by %uname .<br />The folder owner is %fowner .<br />The folder permissions are %perms .', array('%vid_file' => $job->origfile, '%nid' => $job->nid, '%dir' => $dest_dir, '%uname' => $owner, '%fowner' => $fowner, '%perms' => $perms), WATCHDOG_ERROR);
        return FALSE;
      }
    }
    else {
      watchdog('video_conversion', 'Error converting file, no file present.', array(), WATCHDOG_ERROR);
    }
    return FALSE;
  }

  /**
   * Calls the transcoder class to convert the video.
   * 
   * @param $job
   *   Video object to be transcoded
   *   
   * @return
   *   TRUE or FALSE
   */
  private function convert_file(&$job) {
    module_load_include('inc', 'video', '/includes/transcoder');
    $transcoder = new video_transcoder;
    $transcoder->convert_video($job);
    return TRUE;
  }

  /**
   * Load a file based on the file id ($fid)
   * 
   * @param $fid
   *   Integer of the file id to be loaded.
   */
  public function load_video($fid) {
    $result = db_query('SELECT f.* FROM {video_rendering} vr LEFT JOIN {files} f ON vr.fid = f.fid WHERE f.fid=vr.fid AND f.fid = %d', $fid);
    return db_fetch_object($result);
  }

  /**
   * Change the status of the file.
   * 
   * @param (int) $fid
   * @param (int) $status
   */
  public function change_status($fid, $status) {
    $result = db_query('UPDATE {video_rendering} SET status = %d WHERE fid = %d ', $status, $fid);
  }
}
?>