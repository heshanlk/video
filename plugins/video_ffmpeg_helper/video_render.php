<?php
// $Id$

/**
 * @file
 * Manage video rendering scheduling.
 *
 * @author Fabio Varesano <fvaresano at yahoo dot it>
 */
 
 
/**
 * Configuration constats
*/
define(VIDEO_RENDERING_FFMPEG_PATH, '/usr/bin/ffmpeg'); // set to the ffmpeg executable
define(VIDEO_RENDERING_TEMP_PATH, '/tmp/video'); // set to the temp file path

 
/**
 * Define some constants
*/
define(VIDEO_RENDERING_PENGING, 0);
define(VIDEO_RENDERING_ACTIVE, 5);
define(VIDEO_RENDERING_COMPLETE, 10);


include_once './includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

video_render_main();

/**
 * Main for video_render.php
*/
function video_render_main() {
  
  if($job = video_render_select()) {
    video_render_start($job);
  }
  else {
    print 'no jobs to schedule' . "\n";
  }
}


/**
 * Starts rendering for a job
*/
function video_render_start($job) {
  // escape file name for safety
  $videofile = escapeshellarg($job->origfile);
  $convfile = tempnam(VIDEO_RENDERING_TEMP_PATH, 'video-rendering');
  $converter = VIDEO_RENDERING_FFMPEG_PATH;
  $options = preg_replace(array('/%videofile/', '/%convertfile/'), array($videofile, $convfile), variable_get('video_ffmpeg_helper_auto_converter_options', '-y -i %videofile -f flv  %convertfile.flv'));

  $command = "$converter $options";
  
  print('executing ' . $command);
  
  //execute the command
  ob_start();
  passthru($command." 2>&1", $command_return);
  $command_output = ob_get_contents();
  ob_end_clean();
  
  print $command_output;
  
  if (!file_exists($convfile)) {
    print 'video conversion failed';
    // TODO: better error handling
  }
  else {
    // move the video to the definitive location
    $file = array(
      'filename' => $job->origfile . ".flv",
      'filemime' => 'application/octet-stream', // is there something better???
      'filesize' => filesize($convfile),
      'filepath' => $convfile,
      'nid' => $job->nid,
      );

    $file = ((object) $file);

    $dest_dir = variable_get('video_upload_default_path', 'videos') .'/';

    //print file_directory_path() . '/' . $dest_dir . basename($file->filename);
    if (file_move($file, $dest)) {
      $file->fid = db_next_id('{files}_fid');
      print_r($file);
      db_query("INSERT INTO {files} (fid, nid, filename, filepath, filemime, filesize) VALUES (%d, %d, '%s', '%s', '%s', %d)", $file->fid, $job->nid, $file->filename, $file->filepath, $file->filemime, $file->filesize);
      
      db_query("INSERT INTO {file_revisions} (fid, vid, list, description) VALUES (%d, %d, %d, '%s')", $file->fid, $job->vid, $file->list, $file->description);
      
      db_query('UPDATE {video} SET vidfile = "%s" WHERE nid=%d AND vid=%d', $file->filename, $job->nid, $job->vid);
    }
    else {
      print 'error moving video to the final directory';
    }
  }
}


/**
 * Select a job from the queue
*/
function video_render_select() {
  $result = db_query('SELECT * FROM {video_rendering} vr INNER JOIN {node} n ON vr.vid = n.vid INNER JOIN {video} v ON n.vid = v.vid WHERE n.nid = v.nid AND vr.nid = n.nid AND vr.status = %d ORDER BY n.created', VIDEO_RENDERING_PENDING);
  
  return db_fetch_object($result);
}
