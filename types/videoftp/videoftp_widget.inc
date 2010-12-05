<?php
// $Id:

/**
 * @file
 * videoftp widget hooks and callbacks.
 */

/**
 * Implementation of CCK's hook_widget_settings($op = 'form').
 */
function videoftp_widget_settings_form($widget) {
  $form = array();
  $form['file_extensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Permitted upload file extensions.'),
    '#default_value' => !empty($widget['file_extensions']) ? $widget['file_extensions'] : 'mp4 mpeg avi mpg wmv flv mov',
    '#size' => 64,
    '#description' => t('Extensions a user can attach to this field.  Separate extensions with a space and do not include the leading dot.  Leaving this blank will allow attachment of a file with any extension.'),
    '#weight' => 1
  );
  $form['path_settings'] = array(
    '#type' => 'fieldset',
   	'#title' => t('Path settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 2,
  );
  $form['path_settings']['file_path'] = array(
    '#type' => 'textfield',
    '#size' => 64,
    '#title' => t('File path'),
    '#default_value' => ltrim(ltrim($widget['file_path'], "videos"), "/"),
    '#description' => t('Optional subdirectory within the "<em>files/videos/</em>" directory where files will be moved and stored. Do not include preceding or trailing slashes.'),
    '#element_validate' => array('video_widget_settings_file_path_validate', '_filefield_widget_settings_file_path_validate'),
  );  
  $form['path_settings']['ftp_path'] = array(
    '#type' => 'textfield',
    '#title' => t('FTP Filepath'),
    '#default_value' => !empty($widget['ftp_path']) ? $widget['ftp_path'] : 'ftpvideos',
    '#description' => t('The subdirectory within the "<em>files/</em>" directory where you have upload the videos for attachment.  Once the video is attached it will be moved from this directory to the main files directory.'),
    '#required' => TRUE,
    '#weight' => 3,
  );
  //default settings
  $default = video_default_widget_settings($widget);
  $form = $form + $default;
  return $form;
}

/**
 * Implementation of CCK's hook_widget_settings($op = 'validate').
 * 
 * //@todo Integrate this into a universal function to share with uploadfield_widget
 */
function videoftp_widget_settings_validate($widget) {
  // Check that only web images are specified in the callback.
  if(!video_web_extensions($widget['file_extensions'])) {
    form_set_error('file_extensions', t('Only web-standard videos are supported through the videoftp widget. If needing to upload other types of files, change the widget to use a standard file upload.'));
  }
  // Check for our ftp filepath and try to create it, if not it will throw an error.
  $ftp_path = file_directory_path().'/'.$widget['ftp_path'];
  file_check_directory($ftp_path, true, 'ftp_path');
}

/**
 * Implementation of CCK's hook_widget_settings($op = 'save').
 */
function videoftp_widget_settings_save($widget) {
  return array('file_extensions', 'file_path', 'ftp_path', 'autothumbnail','autoconversion', 'default_dimensions', 'default_player_dimensions', 'default_video_thumb');
}

/**
 * The #value_callback for the videoftp_widget type element.
 */
function videoftp_widget_value($element, $edit = FALSE) {
  if (!$edit) {
    // Creating so we load up our empty values.
    $file = field_file_load($element['#default_value']['fid']);
    $item = $element['#default_value'];
  }
  else {
    // Reset our item array for our data.
    $item = array_merge($element['#default_value'], $edit);
    $field = content_fields($element['#field_name'], $element['#type_name']);

    // Uploads take priority over value of fid text field.
    if ($fid = videoftp_save_upload($element)) {
      $item['fid'] = $fid;
      $item['data'] = array(); //reset our thumbnail
    }
    // Check for #videoftp_value_callback values.
    // Because FAPI does not allow multiple #value_callback values like it does
    // for #element_validate and #process, this fills the missing functionality
    // to allow VideoFtp to be extended purely through FAPI.
    elseif (isset($element['#videoftp_value_callback'])) {
      foreach ($element['#videoftp_value_callback'] as $callback) {
        $callback($element, $item);
      }
    }

    // Load file if the FID has changed so that it can be saved by CCK.
    $file = field_file_load($item['fid']);

    // If the file entry doesn't exist, don't save anything.
    if (empty($file)) {
      $item = array();
    }

    // Checkboxes loose their value when empty.
    // If the list field is present make sure its unchecked value is saved.
    if (!empty($field['list_field']) && empty($edit['list'])) {
      $item['list'] = 0;
    }
  }
  // Merge file and item data so it is available to all widgets.
  $item = array_merge($item, $file);
  return $item;
}

/**
 * Process an individual element.
 */
function videoftp_widget_process($element, $edit, &$form_state, $form) {
  $item = $element['#value'];
  $field_name = $element['#field_name'];
  $delta = $element['#delta'];
  $field = content_fields($element['#field_name'], $element['#type_name']);
  $element['#theme'] = 'videoftp_widget_item';
  
  if (isset($element['preview']) && $element['#value']['fid'] != 0) {
    $element['preview']['#value'] = theme('videoftp_widget_preview', $element['#value']);
  }

  // Title is not necessary for each individual field.
  if ($field['multiple'] > 0) {
    unset($element['#title']);
  }

  // Set up the buttons first since we need to check if they were clicked.
  $element['videoftp_attach'] = array(
    '#type' => 'submit',
    '#value' => t('Attach'),
    '#submit' => array('node_form_submit_build_node'),
    '#ahah' => array( // with JavaScript
       'path' => 'videoftp/ahah/'.   $element['#type_name'] .'/'. $element['#field_name'] .'/'. $element['#delta'],
       'wrapper' => $element['#id'] .'-ahah-wrapper',
       'method' => 'replace',
       'effect' => 'fade',
    ),
    '#field_name' => $element['#field_name'],
    '#delta' => $element['#delta'],
    '#type_name' => $element['#type_name'],
    '#upload_validators' => $element['#upload_validators'],
    '#weight' => 100,
    '#post' => $element['#post'],
  );
  $element['videoftp_remove'] = array(
  // With default CCK edit forms, $element['#parents'] is array($element['#field_name'], $element['#delta']).
  // However, if some module (for example, flexifield) places our widget deeper in the tree, we want to
  // use that information in constructing the button name.
    '#name' => implode('_', $element['#parents']) .'_videoftp_remove',
    '#type' => 'submit',
    '#value' => t('Remove'),
    '#submit' => array('node_form_submit_build_node'),
    '#ahah' => array( // with JavaScript
      'path' => 'videoftp/ahah/'.   $element['#type_name'] .'/'. $element['#field_name'] .'/'. $element['#delta'],
      'wrapper' => $element['#id'] .'-ahah-wrapper',
      'method' => 'replace',
      'effect' => 'fade',
    ),
    '#field_name' => $element['#field_name'],
    '#delta' => $element['#delta'],
    '#weight' => 101,
    '#post' => $element['#post'],
  );

  // Because the output of this field changes depending on the button clicked,
  // we need to ask FAPI immediately if the remove button was clicked.
  // It's not good that we call this private function, but
  // $form_state['clicked_button'] is only available after this #process
  // callback is finished.
  if (_form_button_was_clicked($element['videoftp_remove'])) {
    // Delete the file if it is currently unused. Note that field_file_delete()
    // does a reference check in addition to our basic status check.
    if (isset($edit['fid'])) {
      $removed_file = field_file_load($edit['fid']);
      if ($removed_file['status'] == 0) {
        field_file_delete($removed_file);
      }
    }
    $item = array('fid' => 0, 'list' => $field['list_default'], 'data' => array('description' => '', 'video_thumb' => ''));
  }

  // Set access on the buttons and select.
  $element['videoftp_attach']['#access'] = empty($item['fid']);
  $element['videoftp_remove']['#access'] = !empty($item['fid']);
  
  $options = videoftp_options($field);
  $element['ftpselect'] = array(
    '#type' => 'select',
    '#required' => isset($element['#required']) ? $element['#required'] : $field['required'],
    '#options' => $options,
    '#access' => empty($item['fid']),
  );

  // Set the FID.
  $element['fid'] = array(
    '#type' => 'hidden',
    '#value' => $item['fid'],
  );

  if ($item['fid'] != 0) {
    $element['preview'] = array(
      '#type' => 'markup',
      '#value' => theme('video_widget_preview', $item),
    );
  }

  // placeholder.. will be serialized into the data column. this is a place for widgets
  // to put additional data.
  $element['data'] = array(
    '#tree' => 'true',
    '#access' => !empty($item['fid']),
  );

  // Create our thumbnails
  if($field['widget']['autothumbnail']) {
    video_thumb_process($element);
  }
  
  // Add our extra fields if in preview mode
  if(!empty($item['fid'])) {
    video_widget_element_settings($element);
  }

  // Set #element_validate in a way that it will not wipe out other
  // validation functions already set by other modules.
  if (empty($element['#element_validate'])) {
    $element['#element_validate'] = array();
  }
  array_unshift($element['#element_validate'], 'videoftp_widget_validate');

  // Make sure field info will be available to the validator which
  // does not get the values in $form.
  $form_state['#field_info'][$field['field_name']] = $field;

  $element['#attributes']['id'] = $element['#id'] .'-ahah-wrapper';
  $element['#prefix'] = '<div '. drupal_attributes($element['#attributes']) .'>';
  $element['#suffix'] = '</div>';

  // Lets use the clicked_button #submit[0] value here instead and see how that works out for now...
  if($form_state['submitted'] == 1) {
    video_widget_process($element, $form_state);
  }
  return $element;
}

function videoftp_save_upload($element) {
  global $user;
  $upload_name = $element['#field_name'] .'_'. $element['#delta'];
  $delta = $element['#delta'];
  $field = content_fields($element['#field_name'], $element['#type_name']);
  $video = $element['#post'][$field['field_name']][$delta]['ftpselect'];
  $ftp_path = file_directory_path().'/'.$field['widget']['ftp_path'];

  if (empty($video) || !file_exists($ftp_path.'/'.$video)) {
    return 0;
  }
  
  $dest = filefield_widget_file_path($field);
  if (!field_file_check_directory($dest, FILE_CREATE_DIRECTORY)) {
    watchdog('filefield', 'The upload directory %directory for the file field %field (content type %type) could not be created or is not accessible. A newly uploaded file could not be saved in this directory as a consequence, and the upload was canceled.', array('%directory' => $dest, '%field' => $element['#field_name'], '%type' => $element['#type_name']));
    form_set_error($upload_name, t('The file could not be uploaded.'));
    return 0;
  }

  // Begin building the file object.
  $file = new stdClass();
  $file->uid = $user->uid;
  $file->filename = file_munge_filename(trim(basename($video), '.'), $field['widget']['file_extensions']);
  $file->filepath = $ftp_path.'/'.$video;
  $file->filemime = file_get_mimetype($file->filename);
  $file->filesize = filesize($file->filepath);
  $file->status = FILE_STATUS_TEMPORARY;
  $file->timestamp = time();

  //lets move our file from the ftp folder to the files directory
  if(file_move($file, $dest)) {
    // Insert new record to the database.
    drupal_write_record('files', $file);
  }
  _field_file_cache($file); // cache the file in order to minimize load queries
  return $file->fid;
}

function videoftp_options($field) {
  $options = array();
  $options[] = t('Select Video');
  // Lets setup our ftp_path.
  $ftp_path = file_directory_path().'/'.$field['widget']['ftp_path'];
  // We are going to scan the directory and pull out the available video types by extension.
  $extensions = explode(" ",$field['widget']['file_extensions']);
  $video_files = scandir($ftp_path);
  foreach ($video_files as $file) {
    $ext = pathinfo($file);
    if (in_array($ext['extension'], $extensions)) {
      //add the file to the options array for selection
      $options["$file"] = $file;
    }
  }
  return $options;
}