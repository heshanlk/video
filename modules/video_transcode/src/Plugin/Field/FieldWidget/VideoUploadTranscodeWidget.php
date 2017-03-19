<?php

namespace Drupal\video_transcode\Plugin\Field\FieldWidget;

use Drupal\video\Plugin\Field\FieldWidget\VideoUploadWidget;
use Drupal\Core\Form\FormStateInterface;

/**
 * Plugin implementation of the 'video_upload_transcode' widget.
 *
 * @FieldWidget(
 *   id = "video_upload_transcode",
 *   label = @Translation("Video Upload & Convert"),
 *   field_types = {
 *     "video"
 *   }
 * )
 */
class VideoUploadTranscodeWidget extends VideoUploadWidget {

  /**
   * Form API callback.
   *
   * Ensures that a size has been entered and that it can be parsed by
   * \Drupal\Component\Utility\Bytes::toInt().
   *
   * This function is assigned as an #element_validate callback in
   * settingsForm().
   */
  public static function validateMaxFilesize($element, FormStateInterface $form_state) {
    if (!empty($element['#value']) && (Bytes::toInt($element['#value']) == 0)) {
      $form_state->setError($element, t('The option must contain a valid value. You may either leave the text field empty or enter a string like "512" (bytes), "80 KB" (kilobytes) or "50 MB" (megabytes).'));
    }
  }

  /**
   * Form API callback: Processes a video_upload field element.
   *
   * This method is assigned as a #process callback in formElement() method.
   */
  public static function process($element, FormStateInterface $form_state, $form) {
    return parent::process($element, $form_state, $form);
  }
  
}
