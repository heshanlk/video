<?php
// $Id$
/**
 * @file
 * Provides admin functions for the s3 amazon webservices.
 */
function video_s3_admin_settings() {
  $form = array();
  $form['amazon_s3'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Amazon S3.'),
    '#default_value' => variable_get('amazon_s3', FALSE),
    '#description' => t('If you would like to use Amazon S3 to store and serve up your video files enable this and fill out your access keys.'),
  );
  $form['amazon_s3_ssl'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable SSL?'),
    '#default_value' => variable_get('amazon_s3_ssl', FALSE),
    '#description' => t('If you would like to use ssl when transfering your files enable this option.'),
  );
  $form['amazon_s3_access_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Access Key ID'),
    '#default_value' => variable_get('amazon_s3_access_key', ''),
    '#size' => 50,
  );
  $form['amazon_s3_secret_access_key'] = array(
    '#type' => 'password',
    '#title' => t('Secret Access Key'),
    '#default_value' => variable_get('amazon_s3_secret_access_key', ''),
    '#description' => t('Once saved, you do not need to re-enter your secret key.  If you need to update your key, then fill this out to update it.'),
    '#size' => 50,
  );
  //@todo Maybe move this to the admin settings page instead of global?
  $form['amazon_s3_bucket'] = array(
    '#type' => 'textfield',
    '#title' => t('Bucket'),
    '#description' => t('Enter the bucket you wish to store your videos in.  If the bucket doesn\'t exist the system will attempt to create it.'),
    '#default_value' => variable_get('amazon_s3_bucket', ''),
    '#size' => 50,
  );
  $form['#validate'] = array('video_s3_admin_settings_validate');

  //lets show our buckets in table format with a delete link.
  if(variable_get('amazon_s3', FALSE)) { //@todo add permissions
    //were enabled, that means they have successfully connected and created a bucket.
    module_load_include('inc', 'video_s3', '/includes/amazon_s3');
    $s3 = new video_amazon_s3;
    $s3->connect();
    $buckets = $s3->s3->listBuckets();
    // Setup our header.
    $header = array(t('Bucket Name'), t('Total Objects'), t('Actions'));
    $rows = array();
    foreach($buckets as $bucket) {
      $objects = count($s3->s3->getBucket($bucket));
      $actions = l(t('Delete'), 'admin/settings/video/amazon_s3/bucket/'. $bucket .'/delete');
      $rows[] = array($bucket, $objects, $actions);
    }
    $form['amazon_info'] = array(
      '#type' => 'fieldset',
      '#title' => t('Amazon S3 Information'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['amazon_info']['buckets'] = array(
      '#type' => 'markup',
      '#value' => theme('table', $header, $rows),
    );
  }
  return system_settings_form($form);
}

