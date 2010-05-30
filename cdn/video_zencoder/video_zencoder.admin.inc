<?php
// $Id$
/**
 * @file
 * Provides admin functions for the s3 amazon webservices.
 */
function video_zencoder_admin_settings() {
  global $user;

  $form = array();
  $form['amazon_info'] = array(
          '#type' => 'fieldset',
          '#title' => t('Zencoder User Information'),
          '#collapsible' => FALSE,
          '#collapsed' => FALSE,
  );
  $form['amazon_info']['zencoder_username'] = array(
          '#type' => 'textfield',
          '#title' => t('Your email address'),
          '#default_value' => variable_get('zencoder_username', $user->mail),
          '#size' => 50,
  );
  $form['amazon_info']['agree_terms_zencoder'] = array(
          '#type' => 'checkbox',
          '#title' => t('Agree Zencoder Terms and Conditions.'),
          '#default_value' => variable_get('agree_terms_zencoder', TRUE),
          '#description' => t('Read terms and conditions on !link.', array('!link' => l(t('Zencoder.com'), 'http://zencoder.com'))),
  );

  return system_settings_form($form);
}

