<?php
//$Id$
/*
 * @file
 * Class file to handle video conversion using ffmpeg.
 *
 */
define('VIDEO_RENDERING_PENDING', 1);
define('VIDEO_RENDERING_ACTIVE', 5);
define('VIDEO_RENDERING_COMPLETE', 10);
define('VIDEO_RENDERING_FAILED', 20);

class video_conversion {

  /**
   * Our main function to call when converting queued up jobs.
   */
  public function run_queue() {
    if ($videos = $this->select_queue()) {
      foreach ($videos as $video) {
        $this->process($video);
      }
    }
  }

  /**
   * Select videos from our queue
   *
   * @return
   *   An array containing all the videos to be proccessed.
   */
  private function select_queue() {
    $total_videos = variable_get('video_ffmpeg_instances', 5);
    $videos = array();
    $result = db_query_range('SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.status as video_status FROM {video_files} vf LEFT JOIN {files} f ON vf.fid = f.fid WHERE vf.status = %d AND f.status = %d ORDER BY f.timestamp',
    VIDEO_RENDERING_PENDING, FILE_STATUS_PERMANENT, 0, $total_videos);

    while ($row = db_fetch_object($result)) {
      $videos[] = $row;
    }
    return $videos;
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

  private function render($video) {
    if (!is_object($video)) {
      watchdog('video_conversion', 'Video object is not present', array(), WATCHDOG_ERROR);
      return FALSE;
    }
    // Make sure this video is pending or do nothing.
    if($video->video_status == VIDEO_RENDERING_PENDING) {
      // This will update our current video status to active.
      $this->change_status($video->vid, VIDEO_RENDERING_ACTIVE);
      // Get the converted file object
      //we are going to move our video to an "original" folder
      //we are going to transcode the video to the "converted" folder
      $pathinfo = pathinfo($video->filepath);
      $original = $pathinfo['dirname'] .'/original';
      $converted = $pathinfo['dirname'] .'/converted';

      if (!field_file_check_directory($original, FILE_CREATE_DIRECTORY)) {
        watchdog('video_transcoder', 'Video conversion failed.  Could not create the directory: '.$orginal, array(), WATCHDOG_ERROR);
        return false;
      }
      if (!field_file_check_directory($converted, FILE_CREATE_DIRECTORY)) {
        watchdog('video_transcoder', 'Video conversion failed.  Could not create the directory: '.$converted, array(), WATCHDOG_ERROR);
        return false;
      }

      $original = $original .'/'. $video->filename;
      $converted = $converted .'/'. $pathinfo['filename'] .'.flv';
      //lets move our video and then convert it.
      if(file_move($video, $original)) {
        // Update our filepath since we moved it
        $update = drupal_write_record('files', $video, 'fid');
        //call our transcoder
        $command_output = $this->convert_video($video, $converted);
        //lets check to make sure our file exists, if not error out
        if(!file_exists($converted) || !filesize($converted)) {
          watchdog('video_conversion', 'Video conversion failed.  FFMPEG reported the following output: '.$command_output, array(), WATCHDOG_ERROR);
          $this->change_status($video->vid, VIDEO_RENDERING_FAILED);
          return FALSE;
        }
        // Setup our converted video object
        $video_info = pathinfo($converted);
        //update our converted video
        $video->converted = new stdClass();
        $video->converted->vid = $video->vid;
        $video->converted->filename = $video_info['basename'];
        $video->converted->filepath = $converted;
        $video->converted->filemime = file_get_mimetype($converted);
        $video->converted->filesize = filesize($converted);
        $video->converted->status = VIDEO_RENDERING_COMPLETE;
        $video->converted->completed = time();
        //clear our cache so our video path is updated.
        cache_clear_all('*', 'cache_content', true);
        // Update our video_files table with the converted video information.
        $result = db_query("UPDATE {video_files} SET filename='%s', filepath='%s', filemime='%s', filesize=%d, status=%d, completed=%d WHERE vid=%d",
        $video->converted->filename, $video->converted->filepath, $video->converted->filemime, $video->converted->filesize, $video->converted->status, $video->converted->completed, $video->converted->vid);

        // Update our node id to published.  We do not do a node_load as it causes editing problems when saving.
        db_query("UPDATE {node} SET status=%d WHERE nid=%d", 1, $video->nid);
        watchdog('video_conversion', 'Successfully converted %orig to %dest', array('%orig' => $video->filepath, '%dest' => $video->converted->filepath), WATCHDOG_INFO);
        return TRUE;
      }
      else {
        watchdog('video_conversion', 'Cound not move the video to the original folder.', array(), WATCHDOG_ERROR);
        $this->change_status($video->vid, VIDEO_RENDERING_FAILED);
        return FALSE;
      }
    }
    return NULL;
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
  private function convert_video($video, $converted) {
    module_load_include('inc', 'video', '/includes/transcoder');
    $transcoder = new video_transcoder;
    return $transcoder->convert_video($video, $converted);
  }

  /**
   * Load a file based on the file id ($fid)
   *
   * @param $fid
   *   Integer of the file id to be loaded.
   */
  public function load_video($fid) {
    $result = db_query('SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.status as video_status FROM {video_files} vf LEFT JOIN {files} f ON vf.fid = f.fid WHERE f.fid=vf.fid AND f.fid = %d', $fid);
    return db_fetch_object($result);
  }

  /**
   * Load a converted video based on the file id ($fid)
   *
   * @todo: Need to figure something out here for multiple files (HTML 5)
   * @param $fid
   *   Integer of the file id to be loaded.
   */
  public function load_converted_video($fid) {
    $result = db_query('SELECT * FROM {video_files} WHERE fid = %d', $fid);
    return db_fetch_object($result);
  }

  /**
   * Change the status of the file.
   *
   * @param (int) $vid
   * @param (int) $status
   */
  public function change_status($vid, $status) {
    $result = db_query('UPDATE {video_files} SET status = %d WHERE vid = %d ', $status, $vid);
  }
}
?>