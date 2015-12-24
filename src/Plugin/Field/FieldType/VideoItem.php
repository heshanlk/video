<?php

/**
 * @file
 * Contains \Drupal\video\Plugin\Field\FieldType\VideoItem.
 */

namespace Drupal\video\Plugin\Field\FieldType;

use Drupal\Core\Field\FieldDefinitionInterface;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\StreamWrapper\StreamWrapperInterface;
use Drupal\Core\TypedData\DataDefinition;
use Drupal\file\Entity\File;
use Drupal\file\Plugin\Field\FieldType\FileItem;

/**
 * Plugin implementation of the 'video' field type.
 *
 * @FieldType(
 *   id = "video",
 *   label = @Translation("Video"),
 *   description = @Translation("This field stores the ID of an video file or embedded video as an integer value."),
 *   category = @Translation("Reference"),
 *   default_widget = "video_embed",
 *   default_formatter = "video_player",
 *   column_groups = {
 *     "file" = {
 *       "label" = @Translation("File"),
 *       "columns" = {
 *         "target_id", "data"
 *       }
 *     },
 *   },
 *   list_class = "\Drupal\file\Plugin\Field\FieldType\FileFieldItemList",
 *   constraints = {"ReferenceAccess" = {}, "FileValidation" = {}}
 * )
 */
class VideoItem extends FileItem {

  /**
   * The entity manager.
   *
   * @var \Drupal\Core\Entity\EntityManagerInterface
   */
  protected $entityManager;

  /**
   * {@inheritdoc}
   */
  public static function defaultStorageSettings() {
    return array(
      'default_video' => array(
        'uuid' => NULL,
        'data' => NULL
      ),
    ) + parent::defaultStorageSettings();
  }

  /**
   * {@inheritdoc}
   */
  public static function defaultFieldSettings() {
    $settings = array(
      'file_extensions' => 'mp4 ogv webm',
      'file_directory' => 'videos/[date:custom:Y]-[date:custom:m]',
    ) + parent::defaultFieldSettings();
    // Remove the description option.
    unset($settings['description_field']);
    return $settings;
  }

  /**
   * {@inheritdoc}
   */
  public static function schema(FieldStorageDefinitionInterface $field_definition) {
    return array(
      'columns' => array(
        'target_id' => array(
          'description' => 'The ID of the file entity.',
          'type' => 'int',
          'unsigned' => TRUE,
        ),
        'data' => array(
          'description' => "Additional video metadata.",
          'type' => 'varchar',
          'length' => 512,
        ),
      ),
      'indexes' => array(
        'target_id' => array('target_id'),
      ),
      'foreign keys' => array(
        'target_id' => array(
          'table' => 'file_managed',
          'columns' => array('target_id' => 'fid'),
        ),
      ),
    );
  }

  /**
   * {@inheritdoc}
   */
  public static function propertyDefinitions(FieldStorageDefinitionInterface $field_definition) {
    $properties = parent::propertyDefinitions($field_definition);
    
    // unset the default values from the file module
    unset($properties['display']);
    unset($properties['description']);

    $properties['data'] = DataDefinition::create('string')
      ->setLabel(t('Additional video metadata'))
      ->setDescription(t("Additional metadata for the uploadded or embedded video."));

    return $properties;
  }

  /**
   * {@inheritdoc}
   */
  public function storageSettingsForm(array &$form, FormStateInterface $form_state, $has_data) {
    $element = array();

    // We need the field-level 'default_video' setting, and $this->getSettings()
    // will only provide the instance-level one, so we need to explicitly fetch
    // the field.
    $settings = $this->getFieldDefinition()->getFieldStorageDefinition()->getSettings();

    $scheme_options = \Drupal::service('stream_wrapper_manager')->getNames(StreamWrapperInterface::WRITE_VISIBLE);
    $element['uri_scheme'] = array(
      '#type' => 'radios',
      '#title' => t('Upload destination'),
      '#options' => $scheme_options,
      '#default_value' => $settings['uri_scheme'],
      '#description' => t('Select where the final files should be stored. Private file storage has significantly more overhead than public files, but allows restricted access to files within this field.'),
    );

    return $element;
  }

  /**
   * {@inheritdoc}
   */
  public function fieldSettingsForm(array $form, FormStateInterface $form_state) {
    // Get base form from FileItem.
    $element = parent::fieldSettingsForm($form, $form_state);
    // Remove the description option.
    unset($element['description_field']);
    
    return $element;
  }

  /**
   * {@inheritdoc}
   */
  public function preSave() {
    parent::preSave();
  }

  /**
   * {@inheritdoc}
   */
  public static function generateSampleValue(FieldDefinitionInterface $field_definition) {
    // @ TODO : @see the code at core image module
    return array();
  }

  /**
   * {@inheritdoc}
   */
  public function isDisplayed() {
    // Video items do not have per-item visibility settings.
    return TRUE;
  }

  /**
   * Gets the entity manager.
   *
   * @return \Drupal\Core\Entity\EntityManagerInterface.
   */
  protected function getEntityManager() {
    if (!isset($this->entityManager)) {
      $this->entityManager = \Drupal::entityManager();
    }
    return $this->entityManager;
  }

}
