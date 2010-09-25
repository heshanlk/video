<?php
//$Id$
/*
 * @file
 * Transcoder class file to handle ffmpeg_wrapper settings and conversions.
 *
*/


class video_zencoder implements transcoder_interface {
  private $name = 'Zencoder';
  private $value = 'video_zencoder';
  private $is_wsod = true; // web service on demand service


  public function run_command($options) {
    return;
  }

  public function generate_thumbnails($video) {
    global $user;
    // Setup our thmbnail path.
    $video_thumb_path = variable_get('video_thumb_path', 'video_thumbs');
    $final_thumb_path = file_directory_path(). '/' . $video_thumb_path . '/' . $video['fid'];

    // Ensure the destination directory exists and is writable.
    $directories = explode('/', $final_thumb_path);
    // Get the file system directory.
    $file_system = file_directory_path();
    foreach ($directories as $directory) {
      $full_path = isset($full_path) ? $full_path . '/' . $directory : $directory;
      // Don't check directories outside the file system path.
      if (strpos($full_path, $file_system) === 0) {
        field_file_check_directory($full_path, FILE_CREATE_DIRECTORY);
      }
    }

    $files = array();
    // no thumbnails to generate
    $number_of_thumbs = variable_get('no_of_video_thumbs', 5);
    for($i = 0; $i < $number_of_thumbs; $i++) {
      // @TODO Remove hard coded file types
      $filename = $video['fid'] .'_'. sprintf("%04d", $i) . '.png';
      $thumbfile = $final_thumb_path . '/' . $filename;
      //skip files already exists, this will save ffmpeg traffic
      if (!is_file($thumbfile)) {
        $default = drupal_get_path('module', 'video') . '/images/no-thumb.png';
        // Download generated thumbnails from S3.
        if(video_s3_get_object_info($thumbfile))
          $s3_get_object = video_s3_get_object($thumbfile, $thumbfile);
        else {
          $thumbfile = $final_thumb_path . '/no-thumb.png';
          file_copy($default, $thumbfile, FILE_EXISTS_REPLACE);
        }
//          $thumbfile = drupal_get_path('module', 'video') . '/images/no_thumb.gif';
        if (!file_exists($thumbfile)) {
          $error_param = array('%file' => $thumbfile, '%out' => $s3_get_object);
          $error_msg = t("Error downloading thumbnail for video: generated file %file does not exist.<br />S3 Output:<br />%out", $error_param);
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

  public function video_converted_extension() {
    return variable_get('video_zencoder_ext', 'flv');
  }

  public function convert_video($video, $converted) {
    // @TODO :
  }

  /**
   * Return the playtime seconds of a video
   */
  public function get_playtime($video) {
    $ffmpeg_output = ffmpeg_wrapper_file_data($video['filepath']);
    return $ffmpeg_output['duration'];
  }

  /**
   * Return the dimensions of a video
   */
  public function get_dimensions($video) {
    $ffmpeg_output = ffmpeg_wrapper_file_data($video);
    $res = array('width' => 0,'height' => 0);
    if($ffmpeg_output['video']['s']) {
      $dimensions = explode("x", $ffmpeg_output['video']['s']);
      $res['width'] = $dimensions[0] ? $dimensions[0] : NULL;
      $res['height'] = $dimensions[1] ? $dimensions[1] : NULL;
    }
    return $res;
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
    $help = l(t('Zencoder'), 'http://zencoder.com/');
    return $help;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings()
   */
  public function admin_settings() {
    global $user;
    // check amazon s3 module is exists or not
    if(!module_exists('video_s3'))
      drupal_set_message(t('You must enable Video Amazon S3 Module to enable this module.'), 'error');

    $form = array();
    $form['video_zencoder_start'] = array(
      '#type' => 'markup',
      '#value' => '<div id="video_zencoder">',
    );
    $zencoder_api = variable_get('zencoder_api_key', null);
    if(!isset ($zencoder_api) && empty ($zencoder_api)) {
      $form['zencoder_user'] = array(
        '#type' => 'fieldset',
        '#title' => t('Zencoder User'),
        '#collapsible' => FALSE,
        '#collapsed' => FALSE,
        '#description' => t('Save configurations to creare your !link account to transcode and manage your videos using Zencode  API. Once you save your configurations then this will automatically create an account on the Zencoder.com and password and all ther other relevent details will be emailled to you.', array('!link' => l(t('Zencoder.com'), 'http://zencoder.com')))
      );
      $form['zencoder_user']['zencoder_username'] = array(
        '#type' => 'textfield',
        '#title' => t('Your email address'),
        '#default_value' => variable_get('zencoder_username', 'me@localhost'),
        '#size' => 50,
        '#description' => t('Make sure the email is accurate, since we will send all the password details to manage transcoding online and API key details to this.')
      );

      $form['zencoder_user']['agree_terms_zencoder'] = array(
        '#type' => 'checkbox',
        '#title' => t('Agree Zencoder Terms and Conditions.'),
        '#default_value' => variable_get('agree_terms_zencoder', TRUE),
        '#description' => t('Read terms and conditions on !link.', array('!link' => l(t('Zencoder.com'), 'http://zencoder.com'))),
      );
    }
    else {
      // Zencoder API is exists
      $form['zencoder_info'] = array(
        '#type' => 'fieldset',
        '#title' => t('Zencoder API'),
        '#collapsible' => FALSE,
        '#collapsed' => FALSE,
      );
      $form['zencoder_info']['zencoder_api_key'] = array(
        '#type' => 'textfield',
        '#title' => t('Zencoder API Key'),
        '#default_value' => variable_get('zencoder_api_key', ''),
        '#size' => 50,
        '#description' => t('Zencoder API Key.')
      );

      $form['zencoder'] = array(
        '#type' => 'fieldset',
        '#title' => t("Zencoder Settings"),
        '#collapsed' => false,
        '#description' => t('All output settings are optional. We choose sensible defaults for web and iPhone playback.')
      );

      // output video type
      $form['zencoder']['video_zencoder_ext'] = array(
        '#type' => 'textfield',
        '#title' => t('Video Extension'),
        '#default_value' => variable_get('video_zencoder_ext', 'flv'),
        '#size' => 50,
        '#description' => t('The video extensions without the period you want the converted file to contain.')
      );

      // Basic Video Settings
      $form['zencoder']['basic_video'] = array(
        '#type' => 'fieldset',
        '#title' => t("Basic Video Settings"),
        '#collapsed' => true,
        '#collapsible' => true,
        '#description' => t('')
      );

      $form['zencoder']['basic_video']['zc_width'] = array(
        '#type' => 'textfield',
        '#title' => t('Video width'),
        '#default_value' => variable_get('zc_width', ''),
        '#description' => t('Width of the converted video, of the format 600x400. This is the maximum width of the output video specified as a positive integer. In order for video compression to work properly the width should be divisible by 4 (or even better 16).'),
        '#size' => 12,
      );
      $form['zencoder']['basic_video']['zc_height'] = array(
        '#type' => 'textfield',
        '#title' => t('Video height'),
        '#default_value' => variable_get('zc_height', ''),
        '#description' => t('Width of the converted video, of the format 600x400. This is the maximum height of the output video specified as a positive integer. In order for video compression to work properly the height should be divisible by 4 (or even better 16).'),
        '#size' => 12,
      );
      $quality = array(
        1=>'1 - Poor Quality (Smaller file)',
        2=>2,
        3=>'3 - Default',
        4=>4,
        5=>'5 - High Quality (Larger file)'
      );
      $form['zencoder']['basic_video']['zc_quality'] = array(
        '#type' => 'select',
        '#title' => t('Quality'),
        '#options' => $quality,
        '#default_value' => variable_get('zc_quality', 3),
        '#description' => t('This is the desired output video quality. A higher quality setting will mean higher bitrates and higher file sizes. A quality setting of 5 will be nearly lossless and a setting of 1 will be quite compressed and may not look great. Higher quality encoding is also a bit slower than lower quality encoding. As a rule of thumb, lowering quality by a level will reduce file size by about 40%. A quality setting of 3-4 usually looks pretty good. Note that the actual bitrate will vary when using the quality setting, depending on the type of video. Even at the same quality setting, low-complexity video (like a screencast) will generally result in lower bitrates than high-complexity video (like a movie).'),
      );
      $speed = array(
        1=>'1 - Slow (Better Compression)',
        2=>2,
        3=>'3 - Default',
        4=>4,
        5=>'5 - Fast (Worst Compression)'
      );
      $form['zencoder']['basic_video']['zc_speed'] = array(
        '#type' => 'select',
        '#title' => t('Speed'),
        '#options' => $speed,
        '#default_value' => variable_get('zc_speed', 3),
        '#description' => t('This is the desired speed of encoding. A lower speed setting will mean slower encode times, but lower file sizes and better quality video. A high speed setting will transcode quickly, but compression will be less efficient and result in larger files with lower quality output.'),
      );

      // Advaced Video Settings
      $form['zencoder']['adv_video'] = array(
        '#type' => 'fieldset',
        '#title' => t("Advanced Video Settings"),
        '#collapsed' => true,
        '#collapsible' => true,
        '#description' => t('')
      );

      $form['zencoder']['adv_video']['zc_upscale'] = array(
        '#type' => 'checkbox',
        '#title' => t('Upscale?'),
        '#description' => t("If a video is received that is smaller than your maximum frame size (width and height settings), this setting will determine if we will increase the size of the input video to the supplied width and height. For example, if width is set to 640 and the input file is 320x240, the upscale option will force the output video to expand to 640x480. If false, the output video will match the input's original size."),
        '#default_value' => variable_get('zc_upscale', ''),
      );
      $form['zencoder']['adv_video']['zc_stretch'] = array(
        '#type' => 'checkbox',
        '#title' => t('Stretch?'),
        '#description' => t("If true, the aspect ratio of the original file may be distorted to fit the aspect ratio of the supplied width and height. By default, aspect ratio will always be preserved."),
        '#default_value' => variable_get('zc_stretch', ''),
      );
      $form['zencoder']['adv_video']['zc_frame_rate'] = array(
        '#type' => 'textfield',
        '#title' => t('Frame Rate '),
        '#default_value' => variable_get('zc_frame_rate', ''),
        '#description' => t('The output frame rate to use specified as a decimal number (e.g. 15 or 24.98). Unless you need to target a specific frame rate, you might be better off using Maximum Frame Rate setting. By default, the original frame rate will be preserved.'),
        '#size' => 12,
      );
      $form['zencoder']['adv_video']['zc_max_frame_rate'] = array(
        '#type' => 'textfield',
        '#title' => t('Maximum Frame Rate'),
        '#default_value' => variable_get('zc_max_frame_rate', ''),
        '#description' => t('The maximum frame rate to allow specified as a decimal number (e.g. 15 or 24.98). If the original frame rate is lower than this value, it will not be increased. Otherwise, it will be lowered to the max frame rate. By default, the original frame rate will be preserved.'),
        '#size' => 12,
      );
      $form['zencoder']['adv_video']['zc_key_frame_interval'] = array(
        '#type' => 'textfield',
        '#title' => t('Keyframe Interval'),
        '#default_value' => variable_get('zc_key_frame_interval', ''),
        '#description' => t('By default, a keyframe will be created every 250 frames. Specifying a keyframe interval will allow you to create more or less keyframes in your video. Sometimes the number of keyframes can affect video scrubbing. A greater number of keyframes will increase the size of your output file. Keyframe interval should be specified as a positive integer. For example, a value of 100 will create a keyframe every 100 frames.'),
        '#size' => 12,
      );
      $form['zencoder']['adv_video']['zc_vid_bit_rate'] = array(
        '#type' => 'textfield',
        '#title' => t('Video Bitrate'),
        '#default_value' => variable_get('zc_vid_bit_rate', ''),
        '#description' => t('The target video bitrate specified as kilobits per second (Kbps, e.g. 300 or 500). If you set quality, you don\'t need to set bitrate, unless you want to target a specific bitrate.'),
        '#size' => 12,
      );
      $form['zencoder']['adv_video']['zc_bit_rate_cap'] = array(
        '#type' => 'textfield',
        '#title' => t('Bitrate Cap'),
        '#default_value' => variable_get('zc_bit_rate_cap', ''),
        '#description' => t('A bitrate cap specified as kilobits per second (Kbps, e.g. 300 or 500). Specifying a quality alone will vary the bitrate to match the content of the video potentially introducing high peak bitrates. This setting will prevent peaks in the video bitrate, resulting in a file that looks worse but plays more smoothly. We recommend only using this for streaming or for devices that require it.'),
        '#size' => 12,
      );
      $form['zencoder']['adv_video']['zc_buffer_size'] = array(
        '#type' => 'textfield',
        '#title' => t('Buffer Size'),
        '#default_value' => variable_get('zc_buffer_size', ''),
        '#description' => t('The buffer size specified as kilobits per second (Kbps, e.g. 300 or 500). This is another setting useful for playback on specific devices. Check your streaming server or device settings for tips, or set this to the same as the bitrate cap.'),
        '#size' => 12,
      );
      $profile = array(
        'baseline' => 'Baseline - Default',
        'main' => 'Main',
        'high' => 'High - Best for web playback'
      );
      $form['zencoder']['adv_video']['zc_h245_profile'] = array(
        '#type' => 'select',
        '#title' => t('H.264 Profile'),
        '#options' => $profile,
        '#default_value' => variable_get('zc_h245_profile', 1),
        '#description' => t('The H.264 standard defines various sets of capabilities, which are referred to as profiles, targeting specific classes of applications. The currently supported profiles are baseline, main, and high. The baseline profile will produce video that looks worse, but requires less CPU on playback and is required for playback on some devices (e.g. iPhone or iPod). The high profile will look best, but can be CPU intensive to playback. By default, the baseline profile will be used to maximize compatibility.'),
      );
      $level = array(
        '' => '',
        '1' => '1',
        '1.1' => '1.1',
        '1.2' => '1.2',
        '1.3' => '1.3',
        '2' => '2',
        '2.1' => '2.1',
        '2.2' => '2.2',
        '3' => '3',
        '3.1' => '3.1',
        '3.2' => '3.2',
        '4' => '4',
        '4.1' => '4.1',
        '4.2' => '4.2',
        '5' => '5',
        '5.1' => '5.1'
      );
      $form['zencoder']['adv_video']['zc_h245_level'] = array(
        '#type' => 'select',
        '#title' => t('H.264 Level'),
        '#options' => $level,
        '#default_value' => variable_get('zc_h245_level', 0),
        '#description' => t('Similar to profiles, the H.264 standard defines various levels that will limit the size and complexity of the output file. We recommend leaving this blank unless your output device requires it. iPhone video can\'t exceed level 3, Blu-Ray is 4.1, and older iPods max out at 1.3. By default, level 3 will be used to ensure iPhone playback.'),
      );
      $form['zencoder']['adv_video']['zc_skip_video'] = array(
        '#type' => 'checkbox',
        '#title' => t('Skip Video'),
        '#description' => t("The video track will be omitted from the output. You can still specify a video format, however, no video track will be present in the resulting file."),
        '#default_value' => variable_get('zc_skip_video', ''),
      );

      // Advanced Audio Settings
      $form['zencoder']['adv_audio'] = array(
        '#type' => 'fieldset',
        '#title' => t("Advanced Audio Settings"),
        '#collapsed' => true,
        '#collapsible' => true,
        '#description' => t('')
      );
      $audio_codec = array(
        'aac' => 'AAC - Default',
        'mp3' => 'MP3'
      );
      $form['zencoder']['adv_audio']['zc_audio_codec'] = array(
        '#type' => 'select',
        '#title' => t('Audio Codec'),
        '#options' => $audio_codec,
        '#default_value' => variable_get('zc_audio_codec', 'aac'),
        '#description' => t('The audio codec used in the video file can affect the ability to play the video on certain devices. The default codec used is AAC and should only be changed if the playback device you are targeting requires something different.'),
      );
      $form['zencoder']['adv_audio']['zc_audio_bitrate'] = array(
        '#type' => 'textfield',
        '#title' => t('Audio Bitrate'),
        '#default_value' => variable_get('zc_audio_bitrate', ''),
        '#description' => t('The overall audio bitrate specified as kilobits per second (Kbps, e.g. 96 or 160). This value can\'t exceed 160 Kbps per channel. 96-160 is usually a good range for stereo output.'),
        '#size' => 12,
      );
      $audio_channel = array(
        1 => 'Mono',
        2 => 'Stereo - Default'
      );
      $form['zencoder']['adv_audio']['zc_audio_channels'] = array(
        '#type' => 'select',
        '#title' => t('Audio Channels'),
        '#options' => $audio_channel,
        '#default_value' => variable_get('zc_audio_channels', 2),
        '#description' => t('By default we will choose the lesser of the number of audio channels in the input file or 2 (stereo).'),
      );
      $form['zencoder']['adv_audio']['zc_audio_sample_rate'] = array(
        '#type' => 'textfield',
        '#title' => t('Audio Sample Rate'),
        '#default_value' => variable_get('zc_audio_sample_rate', ''),
        '#description' => t('The sample rate of the audio file specified in hertz (e.g. 44100 or 22050). A sample rate of 44100 is the best option for web playback. 22050 and 48000 are also valid options. Warning: the wrong setting may cause encoding problems. By default, 44100 will be used.'),
        '#size' => 12,
      );
      $form['zencoder']['adv_audio']['zc_skip_audio'] = array(
        '#type' => 'checkbox',
        '#title' => t('Skip Audio'),
        '#description' => t("The audio track will be omitted from the output. You must specify a video format and no audio track will be present in the resulting file."),
        '#default_value' => variable_get('zc_skip_audio', ''),
      );

      // Notification Settings
      $form['zencoder']['notification'] = array(
        '#type' => 'fieldset',
        '#title' => t("Notification Settings"),
        '#collapsed' => true,
        '#collapsible' => true,
        '#description' => t('')
      );
      $form['zencoder']['notification']['zc_notify_url'] = array(
        '#type' => 'textfield',
        '#title' => t('URL'),
        '#default_value' => variable_get('zc_notify_url', ''),
        '#description' => t('We will POST a JSON notification to the URL provided. We will consider the notification successful if we receive a 200 OK response.'),
      );
      $form['zencoder']['notification']['zc_notify_email'] = array(
        '#type' => 'textfield',
        '#title' => t('Email Address'),
        '#default_value' => variable_get('zc_notify_email', ''),
        '#description' => t('We will send a notification email to the URL provided. The notification will contain basic file status information.'),
      );


      // Other Settings
      $form['zencoder']['other'] = array(
        '#type' => 'fieldset',
        '#title' => t("Other Settings"),
        '#collapsed' => true,
        '#collapsible' => true,
        '#description' => t('')
      );
      $form['zencoder']['other']['zc_start_clip'] = array(
        '#type' => 'textfield',
        '#title' => t('Start Clip'),
        '#default_value' => variable_get('zc_start_clip', ''),
        '#description' => t('Creates a subclip from the input file, starting at either a timecode or a number of seconds. Format: 0:01:23.6 or 88.6 for 1 minute and 23.6 seconds.'),
        '#size' => 12,
      );
      $form['zencoder']['other']['zc_clip_length'] = array(
        '#type' => 'textfield',
        '#title' => t('Clip Length'),
        '#default_value' => variable_get('zc_clip_length', ''),
        '#description' => t('Creates a subclip from the input file of the specified length using either a timecode or a number of seconds. Format: 0:00:45.3 or 45.3 for 45.3 seconds.'),
        '#size' => 12,
      );
    }
    $form['video_zencoder_end'] = array(
      '#type' => 'markup',
      '#value' => '</div>',
    );
    return $form;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings_validate()
   */
  public function admin_settings_validate($form, &$form_state) {
    $zencoder_api = variable_get('zencoder_api_key', null);
    if(isset ($zencoder_api) && !empty ($zencoder_api) || $form_state['values']['vid_convertor'] != 'video_zencoder')
      return;

    $errors = false;
    // check terms and condition
    if($form_state['values']['agree_terms_zencoder'] == 0) {
      $errors = true;
      form_set_error('agree_terms_zencoder', t('You must agree terms and conditions.' , array()));
    }
    // check for email exists
    // Validate the e-mail address:
    if ($error = user_validate_mail($form_state['values']['zencoder_username'])) {
      $errors = true;
      form_set_error('zencoder_username', $error);
    }

    // get the API key from zencoder and save it to variable
    if(!$errors) {
      $email = $form_state['values']['zencoder_username'];
      module_load_include('inc', 'video_zencoder', '/includes/zencoder');
      $zc = new video_zencoder_api;
//    if(!($error = $zc->create_user($user)))
//            form_set_error('zencoder_username', $error);
      $user = new stdClass;
      $user->email = $email;
      $result = $zc->create_user($user);
      if($result !== true)
        form_set_error('zencoder_username', $result);
    }
  }

  public function create_job($video) {
    return db_query("INSERT INTO {video_files} (fid, status, dimensions) VALUES (%d, %d, '%s')", $video['fid'], VIDEO_RENDERING_PENDING, $video['dimensions']);
  }

  public function update_job($video) {
    //lets update our table to include the nid
    db_query("UPDATE {video_files} SET nid=%d WHERE fid=%d", $form_state['nid'], $fid);
  }

  public function delete_job($video) {
    //lets get all our videos and unlink them
    $sql = db_query("SELECT filepath FROM {video_files} WHERE fid=%d", $video->fid);
    //we loop here as future development will include multiple video types (HTML 5)
    while ($row = db_fetch_object($sql)) {
      if (file_exists($row->filepath))
        unlink($row->filepath);
    }
    //now delete our rows.
    db_query('DELETE FROM {video_files} WHERE fid = %d', $file->fid);
  }

  public function load_job($fid) {
    $result = db_query('SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.status as video_status FROM {video_files} vf LEFT JOIN {files} f ON vf.fid = f.fid WHERE f.fid=vf.fid AND f.fid = %d', $fid);
    return db_fetch_object($result);
  }

  public function load_job_queue() {
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
   * Change the status of the file.
   *
   * @param (int) $vid
   * @param (int) $status
   */
  public function change_status($vid, $status) {
    $result = db_query('UPDATE {video_files} SET status = %d WHERE vid = %d ', $status, $vid);
  }

  /*
   * Function determines the dimensions you want and compares with the actual wxh of the video.
   *
   * If they are not exact or the aspect ratio does not match, we then figure out how much padding
   * we should add.  We will either add a black bar on the top/bottom or on the left/right.
   *
   * @TODO I need to look more at this function.  I don't really like the guess work here.  Need to implement
   * a better way to check the end WxH.  Maybe compare the final resolution to our defaults?  I don't think
   * that just checking to make sure the final number is even is accurate enough.
   */

  public function dimensions($video) {
    //lets setup our dimensions.  Make sure our aspect ratio matches the dimensions to be used, if not lets add black bars.
    $aspect_ratio = _video_aspect_ratio($video->filepath);
    $ratio = $aspect_ratio['ratio'];
    $width = $aspect_ratio ['width'];
    $height = $aspect_ratio['height'];

    $wxh = explode('x', $video->dimensions);
    $output_width = $wxh[0];
    $output_height = $wxh[1];
    $output_ratio = number_format($output_width / $output_height, 4);

    if ($output_ratio != $ratio && $width && $height) {
      $options = array();
      // Figure out our black bar padding.
      if ($ratio < $output_width / $output_height) {
        $end_width = $output_height * $ratio;
        $end_height = $output_height;
      } else {
        $end_height = $output_width / $ratio;
        $end_width = $output_width;
      }

      // We need to get back to an even resolution and maybe compare with our defaults?
      // @TODO Make this more exact on actual video dimensions instead of making sure the wxh are even numbers

      if ($end_width == $output_width) {
        // We need to pad the top/bottom of the video
        $padding = round($output_height - $end_height);
        $pad1 = $pad2 = floor($padding / 2);
        if ($pad1 % 2 !== 0) {
          $pad1++;
          $pad2--;
        }
        $options[] = '-padtop ' . $pad1;
        $options[] = '-padbottom ' . $pad2;
      } else {
        // We are padding the left/right of the video.
        $padding = round($output_width - $end_width);
        $pad1 = $pad2 = floor($padding / 2);  //@todo does padding need to be an even number?
        if ($pad1 % 2 !== 0) {
          $pad1++;
          $pad2--;
        }
        $options[] = '-padleft ' . $pad1;
        $options[] = '-padright ' . $pad2;
      }

      $end_width = round($end_width) % 2 !== 0 ? round($end_width) + 1 : round($end_width);
      $end_height = round($end_height) % 2 !== 0 ? round($end_height) + 1 : round($end_height);
      //add our size to the beginning to make sure it hits our -s
      array_unshift($options, $end_width . 'x' . $end_height);
      return implode(' ', $options);
    } else {
      return $video->dimensions;
    }
  }
}