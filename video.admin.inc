<?php
/**
 * @file
 * Provides the administration settings for the Video Drupal module.
 */
video_add_adminjs();

/**
 * Video general admin settings
 */
function video_general_admin_settings() {
  $form = array();
  $form['video_autoplay'] = array(
    '#type' => 'checkbox',
    '#title' => t('Automatically start video on page load'),
    '#default_value' => variable_get('video_autoplay', FALSE),
    '#description' => t('Start the video when the page and video loads')
  );
  $form['video_autobuffering'] = array(
    '#type' => 'checkbox',
    '#title' => t('Automatically start video buffering'),
    '#default_value' => variable_get('video_autobuffering', TRUE),
    '#description' => t('Start buffering video when the page and video loads')
  );
  $form['video_bypass_conversion'] = array(
    '#type' => 'checkbox',
    '#title' => t('Bypass auto conversion by default'),
    '#default_value' => variable_get('video_bypass_conversion', FALSE),
    '#description' => t('Bypass video conversion when creating videos. This setting can be overriden by users with the %perm permission.', array('%checkbox' => t('Bypass auto conversion'), '%perm' => t('bypass conversion video')))
  );
  $form['video_convert_on_save'] = array(
    '#type' => 'checkbox',
    '#title' => t('Video convert on node submit'),
    '#default_value' => variable_get('video_convert_on_save', FALSE),
    '#description' => t('Convert videos on node submit.')
  );
  $form['video_use_default_thumb'] = array(
    '#type' => 'checkbox',
    '#title' => t('Override auto thumbnails with default'),
    '#default_value' => variable_get('video_use_default_thumb', FALSE),
    '#description' => t('Override auto thumbnails with default thumbnail.')
  );
  $form['video_dimensions'] = array(
    '#type' => 'textarea',
    '#title' => t('Available dimensions when uploading videos'),
    '#description' => t('Enter one dimension per line. Each resolution must be in the form of <code>width</code>x<code>height</code>. Example dimensions: <code>1280x720</code>.'),
    '#default_value' => variable_get('video_dimensions', video_default_dimensions()),
    '#wysiwyg' => FALSE,
  );
  return system_settings_form($form);
}

/**
 * Video player admin settings
 */
function video_players_admin_settings($form_state) {
  // Check for SWF Tools and Amazon S3 Private URLs
  if (module_exists('video_s3') && module_exists('swftools') && variable_get('vid_filesystem', NULL) == 'video_s3' && variable_get('amazon_s3_private', FALSE)) {
    drupal_set_message(t('Using the Flowplayer FLV player via SWF Tools does not work when the S3 %setting-name setting is turned on. Install and enable the <a href="@flowplayer-api-url">Flowplayer API</a> module to use Flowplayer.', array('%setting-name' => t('Enable private file storage'), '@flowplayer-api-url' => 'http://drupal.org/project/flowplayer')), 'warning');
  }

  $form = array();
  $form['extensions'] = array(
    '#type' => 'fieldset',
    '#title' => t('Video extensions'),
    '#description' => t('Here you can map specific players to each video extension type.'),
  );
  // Lets get all our supported extensions and players.
  $extensions = video_video_extensions();
  $players = video_video_players();
  $flv_players = video_video_flv_players();
  $html5_players = video_video_html5_players();

  foreach ($extensions as $ext => $player) {
    $form['extensions']['video_extension_' . $ext] = array(
      '#type' => 'select',
      '#title' => t('Extension:') . '  ' . $ext,
      '#default_value' => variable_get('video_extension_' . $ext, $player),
      '#options' => $players,
      '#prefix' => '<div class="video_select" rel="' . $ext . '">',
      '#suffix' => '</div>',
    );

    $form['extensions']['video_extension_' . $ext . '_flash_player'] = array(
      '#type' => !empty($flv_players) ? 'radios' : 'markup',
      '#title' => t('Flash player for') . ' ' . $ext,
      '#value' => !empty($flv_players) ? '' : t('No flash players detected.<br />You need to install !swf_tools or !flowplayer.', array('!swf_tools' => l(t('SWF Tools'), 'http://www.drupal.org/project/swftools'), '!flowplayer' => l(t('Flowplayer'), 'http://www.drupal.org/project/flowplayer'))),
      '#options' => $flv_players,
      '#default_value' => variable_get('video_extension_' . $ext . '_flash_player', ''),
      '#prefix' => '<div class="admin_flv_player_wrapper" id="flv_player_' . $ext . '">',
      '#suffix' => '</div>',
    );
    $form['extensions']['video_extension_' . $ext . '_html5_player'] = array(
      '#type' => !empty($html5_players) ? 'radios' : 'markup',
      '#title' => t('HTML5 player for') . ' ' . $ext,
      '#value' => !empty($html5_players) ? '' : t('No HTML5 players detected.<br />You need to install !videojs.', array('!videojs' => l(t('Video.js'), 'http://www.drupal.org/project/videojs'), '!flowplayer' => l(t('Flowplayer'), 'http://www.drupal.org/project/flowplayer'))),
      '#options' => $html5_players,
      '#default_value' => variable_get('video_extension_' . $ext . '_html5_player', ''),
      '#prefix' => '<div class="admin_html5_player_wrapper" id="html5_player_' . $ext . '">',
      '#suffix' => '</div>',
    );
  }

  $form['extra'] = array(
    '#type' => 'fieldset',
    '#title' => t('Player settings'),
  );

  if (isset($flv_players['flowplayer'])) {
    $form['extra']['video_flowplayer_extraplayerheight'] = array(
      '#type' => 'textfield',
      '#title' => t('Flowplayer: add vertical space to accommodate the control bar'),
      '#field_suffix' => 'pixels',
      '#default_value' => variable_get('video_flowplayer_extraplayerheight', 24),
      '#description' => t('The control bar in Flowplayer 3.2 and up uses an overlay on top of the video, so the player size is equal to the video size. For custom control bars or older Flowplayer versions you can use this field to add vertical space to the player height.'),
      '#size' => 5,
      '#maxlength' => 3,
      '#element_validate' => array('_video_validate_number'),
    );
  }

  if (count($form['extra']) == 2) {
    unset($form['extra']);
  }

  return system_settings_form($form);
}

/**
 * Video transcoder admin settings
 */
function video_transcoder_admin_settings($form_state) {
  $form = array();

  $transcoderplugins = video_get_transcoder_plugins();

  ksort($transcoderplugins);

  $transcoders = array();
  $help = array();

  foreach ($transcoderplugins as $transcoderplugin) {
    $transcoder = video_get_transcoder($transcoderplugin['name']);

    $transcoders[$transcoderplugin['name']] = $transcoder->get_name();
    $help[] = $transcoder->get_help();
    $form = array_merge($form, $transcoder->admin_settings($form_state));
  }

  $form['vid_convertor'] = array(
    '#type' => 'radios',
    '#title' => t('Video transcoder'),
    '#default_value' => variable_get('vid_convertor', 'video_localcommand'),
    '#options' => $transcoders,
    '#description' => t('Selecting a video transcoder will help you convert videos and generate thumbnails. !list', array('!list' => theme('item_list', $help))),
    '#prefix' => '<div id="transcoder-radios">',
    '#suffix' => '</div>',
    '#weight' => -1,
  );

  return system_settings_form($form);
}

/**
 * Form API callback to validate the upload settings form.
 */
function video_transcoder_admin_settings_validate($form, &$form_state) {
  $transcoder = video_get_transcoder($form_state['values']['vid_convertor']);
  $transcoder->admin_settings_validate($form, $form_state);
}

/**
 * Video transcoder admin settings
 */
function video_preset_admin_settings($form_state) {
  $presets = video_get_presets(TRUE);
  $enabled = video_get_presets();

  $options = array();
  $help = array();
  foreach ($presets as $preset) {
    $options[$preset->id] = check_plain($preset->name);

    if (!empty($preset->help)) {
      $help[$preset->id] = check_plain($preset->help);
    }
  }

  $form['vid_preset'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Video transcode presets'),
    '#options' => $options,
    '#default_value' => array_keys($enabled),
    '#required' => TRUE,
    '#help' => $help,
  );

  $form = system_settings_form($form);
  $form['#theme'] = 'video_preset_admin_settings';
  return $form;
}

function theme_video_preset_admin_settings($form) {
  foreach ($form['vid_preset']['#options'] as $key => $value) {
    if (!empty($form['vid_preset']['#help'][$key])) {
      $form['vid_preset'][$key]['#suffix'] = '<small>'. $form['vid_preset']['#help'][$key] .'</small>';
    }

    $selected = $form['vid_preset'][$key]['#default_value'];

    $rows[] = array(
      'data' => array(
        drupal_render($form['vid_preset'][$key]),
        l(t('Configure'), 'admin/settings/video/presets/'. rawurlencode($key) .'/configure'),
        l(t('Edit'), 'admin/settings/video/presets/'. rawurlencode($key) .'/edit'),
        $selected ? '' : l(t('Delete'), 'admin/settings/video/presets/'. rawurlencode($key) .'/delete'),
      ),
      'class' => $selected ? 'selected' : '',
    );
  }
  $header = array(
    theme('table_select_header_cell') + array('data' => ' '. t('Preset')),
    array('data' => t('Operations'), 'colspan' => 3),
  );
  unset($form['vid_preset']['#title']);
  $output = theme('table', $header, $rows);
  $output .= drupal_render($form);

  return $output;
}

function video_preset_admin_settings_validate($form, &$form_state) {
  $presets =& $form_state['values']['vid_preset'];
  $presets = array_diff($presets, array(0));
  $presets = array_values($presets);
}

function video_admin_preset($form_state, video_preset $preset = NULL) {
  $form = array(
    '#redirect' => 'admin/settings/video/presets',
    '#preset' => $preset,
  );

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#required' => TRUE,
    '#maxlength' => 50,
    '#default_value' => $preset == NULL ? '' : $preset->name,
  );

  $form['help'] = array(
    '#type' => 'textarea',
    '#title' => t('Help text'),
    '#required' => FALSE,
    '#default_value' => $preset == NULL ? '' : $preset->help,
    '#rows' => 3,
    '#wysiwyg' => FALSE,
  );

  $extensions = array_keys(video_video_extensions());
  $extensions = array_combine($extensions, $extensions);
  $form['extension'] = array(
    '#type' => 'select',
    '#title' => t('File extension'),
    '#required' => TRUE,
    '#options' => $extensions,
    '#default_value' => $preset == NULL ? '' : $preset->extension,
    '#description' => t('Give the extension that the converted file will have. Use only lowercase alphanumeric characters. Do not enter a dot.'),
  );

  $form['filenamesuffix'] = array(
    '#type' => 'textfield',
    '#title' => t('File name suffix'),
    '#required' => FALSE,
    '#maxlength' => 15,
    '#size' => 10,
    '#default_value' => $preset == NULL ? '' : $preset->filenamesuffix,
    '#description' => t('If you have multiple presets with the same extension, you must use this field to differentiate between converted files with the same extension. This suffix is added after the file name and before the extension. For instance, if you enter &quot;_high&quot; and extension &quot;flv&quot;, sample.mp4 will be converted to sample_high.flv. Use only alphanumeric characters, . , - or _.'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
    '#suffix' => l(t('Back'), $form['#redirect']),
  );

  return $form;
}

function video_admin_preset_validate($form, $form_state) {
  $v = $form_state['values'];
  $preset = $form['#preset'];

  if (!empty($v['extension'])) {
    if (!preg_match('#^[a-z0-9]{1,5}$#', $v['extension'])) {
      form_error($form['extension'], t('Use only lowercase alphanumeric characters for the file extension.'));
    }
  }

  if (!empty($v['filenamesuffix'])) {
    if (!preg_match('#^[a-zA-Z0-9\-_.]{1,15}$#', $v['filenamesuffix'])) {
      form_error($form['filenamesuffix'], t('Use only alphanumeric characters, . , - or _ for the file name suffix.'));
    }
  }

  if (!empty($v['help']) && drupal_strlen($v['help']) > 200) {
    form_error($form['help'], t('The maximum length for the help text is 200 characters.'));
  }

  if (!empty($v['extension'])) {
    $presetid = $preset == NULL ? 0 : $preset->id;
    $result = db_fetch_object(db_query('SELECT name FROM {video_preset} WHERE extension = "%s" AND filenamesuffix = "%s" AND id <> %d', $v['extension'], $v['filenamesuffix'], $presetid));
    if ($result) {
      form_error($form['extension'], t('The given extension and file name suffix are already used by preset @preset. Change the extension and file name suffix to something unique.', array('@preset' => $result->name)));
    }
  }
}

function video_admin_preset_submit($form, $form_state) {
  $v = $form_state['values'];
  $preset = $form['#preset'];

  if (!$preset) {
    $preset = new stdClass();
    $update = array();
  }
  else {
    $update = 'id';
  }

  $preset->name = $v['name'];
  $preset->help = $v['help'];
  $preset->extension = $v['extension'];
  $preset->filenamesuffix = $v['filenamesuffix'];

  drupal_write_record('video_preset', $preset, $update);
}

function video_admin_preset_delete($form_state, video_preset $preset) {
  $form = array(
    '#redirect' => 'admin/settings/video/presets',
    '#preset' => $preset,
  );

  $form['text'] = array(
    '#type' => 'item',
    '#value' => t('Are you sure you want to delete preset @preset? All associated settings and transcoder configurations will be lost.', array('@preset' => $preset->name)),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
    '#suffix' => l(t('Back'), $form['#redirect']),
  );

  return $form;
}

function video_admin_preset_delete_validate($form, $form_state) {
  $preset = $form['#preset'];

  // Selected presets can't be deleted
  $selected = video_get_presets();
  if (isset($selected[$preset->id])) {
    form_error($form['text'], t('You are not allowed to delete presets that are currently active.'));
  }
}

function video_admin_preset_delete_submit($form, $form_state) {
  $preset = $form['#preset'];

  db_query('DELETE FROM {video_preset_settings} WHERE presetid = %d', $preset->id);
  db_query('DELETE FROM {video_preset} WHERE id = %d', $preset->id);
}

/**
 * Form for transcoder specific preset settings
 */
function video_admin_preset_configure(&$form_state, video_preset $preset) {
  $transcoder = video_get_transcoder();
  $presetform = $transcoder->preset_settings($form_state, $preset);

  $form_state['preset'] = $preset;
  $form_state['transcoder'] = $transcoder;

  // Save the settings keys that are saved to the database later
  $form_state['keys'] = array();
  foreach (element_children($presetform) as $child) {
    $type = '';
    if (isset($presetform[$child]['#type'])) {
      $type = $presetform[$child]['#type'];
    }

    if ($type != 'item' && $type != 'value' && $type != 'markup') {
      $form_state['keys'][] = $child;
    }
  }

  $form = array(
    '#redirect' => 'admin/settings/video/presets',
  );

  $form['preset'] = array(
    '#type' => 'item',
    '#title' => t('Preset'),
    '#value' => check_plain($preset->name) .' ('. t('extension:') .' '. $preset->extension .')',
  );

  $form['transcoder'] = array(
    '#type' => 'item',
    '#title' => t('Transcoder'),
    '#value' => check_plain($transcoder->get_name()),
  );

  if (!empty($form_state['keys'])) {
    $form = array_merge($form, $presetform);

    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
      '#suffix' => l(t('Back'), $form['#redirect']),
    );
  }
  else {
    $form['notice'] = array(
      '#type' => 'item',
      '#value' => t('This transcoder does not have specific settings for this preset.'),
      '#suffix' => l(t('Back'), $form['#redirect']),
    );
  }

  return $form;
}

function video_admin_preset_configure_validate($form, &$form_state) {
  $form_state['transcoder']->preset_settings_validate($form, $form_state);
}

function video_admin_preset_configure_submit($form, &$form_state) {
  $values = $form_state['values'];
  $settings = array();

  foreach ($form_state['keys'] as $key) {
    if (isset($values[$key]) && $values[$key] !== '') {
      $settings[$key] = $values[$key];
    }
  }

  $form_state['preset']->setSettings($settings);
}

/**
 * File system admin settings
 */
function video_filesystem_admin_settings($form_state) {
  $form = array();

  $filesystemplugins = video_get_filesystem_plugins();

  ksort($filesystemplugins);

  $filesystems = array();
  $help = array();

  foreach ($filesystemplugins as $filesystemplugin) {
    $filesystem = video_get_filesystem($filesystemplugin['name']);

    $filesystems[$filesystemplugin['name']] = $filesystem->get_name();
    $help[] = $filesystem->get_help();

    $form['video_'. $filesystemplugin['name'] .'_start'] = array(
      '#type' => 'markup',
      '#value' => '<div id="'. $filesystemplugin['name'] .'">',
    );
    $form = array_merge($form, $filesystem->admin_settings($form_state));
    $form['video_'. $filesystemplugin['name'] .'_end'] = array(
      '#type' => 'markup',
      '#value' => '</div>',
    );
  }

  $form['vid_filesystem'] = array(
    '#type' => 'radios',
    '#title' => t('Video file system'),
    '#default_value' => variable_get('vid_filesystem', 'drupal'),
    '#options' => $filesystems,
    '#description' => theme('item_list', $help),
    '#prefix' => '<div id="filesystem-radios">',
    '#suffix' => '</div>',
    '#weight' => -1,
  );

  return system_settings_form($form);
}

function video_filesystem_admin_settings_validate($form, &$form_state) {
  $filesystem = video_get_filesystem($form_state['values']['vid_filesystem']);
  $filesystem->admin_settings_validate($form, $form_state);
}

/**
 * Video cron admin settings
 */
function video_cron_admin_settings() {
  $form = array();
  $form['video_cron'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use Drupals built in cron.'),
    '#default_value' => variable_get('video_cron', TRUE),
    '#description' => t('If you would like to use Drupals built in cron hook, check this box.  Please be warned that transcoding videos is very resource intensive.  If you use poor mans cron, I highly discourage this option.  I also suggest you setup your cron to call this function through CLI instead of WGET.'),
  );
  $form['video_queue_batchsize'] = array(
    '#type' => 'textfield',
    '#title' => t('Total videos to convert during each cron process.'),
    '#default_value' => variable_get('video_queue_batchsize', 5),
    '#description' => t('How many videos do you want to process on each cron run? Either through Drupal cron, Drush or the video_scheduler.php.'),
  );
  return system_settings_form($form);
}

/**
 * Return our possible flash players.
 */
function video_video_flv_players() {
  $options = array();
  if (module_exists('swftools')) {
    $options['swftools'] = t('SWF Tools');
  }
  if (module_exists('flowplayer')) {
    $options['flowplayer'] = t('Flowplayer');
  }
  return $options;
}

/**
 * Return our possible html5 players.
 */
function video_video_html5_players() {
  $options = array(
    'video' => t('Video (default)'),
  );

  if (module_exists('videojs')) {
    $options['videojs'] = t('Video.js');
  }

  return $options;
}

function _video_validate_number($element, $form_state) {
  $key = end($element['#parents']);
  $val = $form_state['values'][$key];

  if (!empty($val) && (!preg_match('#^\d+$#', $val) || intval($val) < 0)) {
    form_error($element, t('You must enter an integer value for field %field.', array('%field' => $element['#title'])));
  }
}
