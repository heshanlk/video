<?php

namespace Drupal\video_transcode\Entity;

use Drupal\Core\Entity\EntityStorageInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\video_transcode\TranscodeJobInterface;
use Drupal\user\UserInterface;
use Drupal\Core\Entity\EntityChangedTrait;
use Drupal\Core\Url;
use Drupal\Core\Link;

/**
 * Defines the ContentEntityType entity.
 *
 * @ContentEntityType(
 *   id = "video_transcode_job",
 *   label = @Translation("Transcode job entity"),
 *   handlers = {
 *     "view_builder" = "Drupal\Core\Entity\EntityViewBuilder",
 *     "list_builder" = "Drupal\video_transcode\Controller\TranscodeJobListBuilder",
 *     "form" = {
 *       "add" = "Drupal\video_transcode\Form\TranscodeJobForm",
 *       "edit" = "Drupal\video_transcode\Form\TranscodeJobForm",
 *       "delete" = "Drupal\video_transcode\Form\TranscodeJobDeleteForm",
 *     },
 *     "access" = "Drupal\video_transcode\TranscodeJobAccessControlHandler",
 *   },
 *   list_cache_contexts = { "user" },
 *   base_table = "video_transcode_job",
 *   admin_permission = "administer video_transcode_job entity",
 *   entity_keys = {
 *     "id" = "id",
 *     "label" = "name",
 *     "uuid" = "uuid"
 *   },
 *   links = {
 *     "canonical" = "/admin/structure/transcode-jobs/{video_transcode_job}",
 *     "edit-form" = "/admin/config/media/transcode-jobs/{video_transcode_job}/edit",
 *     "delete-form" = "/admin/config/media/transcode-jobs/{video_transcode_job}/delete",
 *     "collection" = "/admin/config/media/transcode-jobs/list"
 *   },
 *   field_ui_base_route = "video_transcode.video_transcode_job_settings",
 *   common_reference_target = TRUE
 * )
 *
 */
class TranscodeJob extends ContentEntityBase implements TranscodeJobInterface {

  use EntityChangedTrait;

  /**
   * {@inheritdoc}
   *
   * When a new entity instance is added, set the user_id entity reference to
   * the current user as the creator of the instance.
   */
  public static function preCreate(EntityStorageInterface $storage_controller, array &$values) {
    parent::preCreate($storage_controller, $values);
    $values += array(
      'user_id' => \Drupal::currentUser()->id(),
    );
  }

  /**
   * {@inheritdoc}
   */
  public function getCreatedTime() {
    return $this->get('created')->value;
  }

  /**
   * {@inheritdoc}
   */
  public function getChangedTime() {
    return $this->get('changed')->value;
  }

  /**
   * {@inheritdoc}
   */
  public function getOwner() {
    return $this->get('user_id')->entity;
  }

  /**
   * {@inheritdoc}
   */
  public function getOwnerId() {
    return $this->get('user_id')->target_id;
  }

  /**
   * {@inheritdoc}
   */
  public function setOwnerId($uid) {
    $this->set('user_id', $uid);
    return $this;
  }

  /**
   * {@inheritdoc}
   */
  public function setOwner(UserInterface $account) {
    $this->set('user_id', $account->id());
    return $this;
  }

  /**
   * {@inheritdoc}
   *
   * Define the field properties here.
   *
   * Field name, type and size determine the table structure.
   *
   * In addition, we can define how the field and its content can be manipulated
   * in the GUI. The behaviour of the widgets used can be determined here.
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type) {

    // Standard field, used as unique if primary index.
    $fields['id'] = BaseFieldDefinition::create('integer')
      ->setLabel(t('ID'))
      ->setDescription(t('The ID of the Transcode Job entity.'))
      ->setReadOnly(TRUE);

    // Standard field, unique outside of the scope of the current project.
    $fields['uuid'] = BaseFieldDefinition::create('uuid')
      ->setLabel(t('UUID'))
      ->setDescription(t('The UUID of the Transcode Job entity.'))
      ->setReadOnly(TRUE);

    // Needed as label of entity
    $fields['name'] = BaseFieldDefinition::create('string')
      ->setLabel(t('Name'))
      ->setDescription(t('The Name of the Transcode Job entity.'))
      ->setReadOnly(TRUE);

    // Input file for the transcode job.
    $fields['input_file'] = BaseFieldDefinition::create('video')
      ->setLabel(t('Input File'))
      ->setDescription(t('The input file for transcode job.'))
      ->setDisplayOptions('view', array(
        'label' => 'above',
        'type' => 'video_player_list',
        'weight' => -6,
      ))
      ->setDisplayOptions('form', array(
        'type' => 'video_upload',
        'weight' => -6,
      ))
      ->setRequired(TRUE)
      ->setDisplayConfigurable('form', TRUE)
      ->setDisplayConfigurable('view', TRUE);

      // Transcoder field for the transcode job.
      $fields['transcoder'] = BaseFieldDefinition::create('list_string')
        ->setLabel(t('Transcoder'))
        ->setDescription(t('The transcoder.'))
        ->setSettings(array(
          'allowed_values' => array(
            'zencoder' => 'Zencoder',
            'ffmpeg' => 'FFMpeg',
          ),
        ))
        ->setDisplayOptions('view', array(
          'label' => 'above',
          'type' => 'string',
          'weight' => -5,
        ))
        ->setDisplayOptions('form', array(
          'type' => 'options_buttons',
          'weight' => -5,
        ))
        ->setRequired(TRUE)
        ->setDisplayConfigurable('form', TRUE)
        ->setDisplayConfigurable('view', TRUE);

      // Transcoder field for the transcode job.
      $preset_ids = \Drupal::entityQuery('video_transcode_preset')->execute();
      $presets = \Drupal::entityTypeManager()->getStorage('video_transcode_preset')->loadMultiple($preset_ids);
      $preset_options = array();
      foreach($presets as $key => $preset){
        $preset_options[$key] = $preset->label;
      }
      if(empty($preset_options)){
        $preset_options['_none'] = t('None - Please create a Video Preset @here.', ['@here' => Link::fromTextAndUrl(t('here'), Url::fromUri('internal:/admin/config/media/transcode-preset'))]);
      }
      $fields['transcoder_preset'] = BaseFieldDefinition::create('list_string')
        ->setLabel(t('Transcoder preset'))
        ->setDescription(t('The transcoder presets to use for output.'))
        ->setSettings(array(
          'allowed_values' => $preset_options,
        ))
        ->setDisplayOptions('view', array(
          'label' => 'above',
          'type' => 'string',
          'weight' => -5,
        ))
        ->setDisplayOptions('form', array(
          'type' => 'options_buttons',
          'weight' => -5,
        ))
        ->setCardinality(-1)
        ->setRequired(TRUE)
        ->setDisplayConfigurable('form', TRUE)
        ->setDisplayConfigurable('view', TRUE);

      // Outpu files from the transcode job.
      $fields['output_files'] = BaseFieldDefinition::create('video')
        ->setLabel(t('Output Files'))
        ->setDescription(t('The output files from them transcode job.'))
        ->setDisplayOptions('view', array(
          'label' => 'above',
          'type' => 'video_player_list',
          'weight' => -4,
        ))
        ->setDisplayOptions('form', array(
          'type' => 'video_upload',
          'weight' => -4,
        ))
        ->setCardinality(-1)
        ->setDisplayConfigurable('form', TRUE)
        ->setDisplayConfigurable('view', TRUE);

      // Thumbnail file from the transcode job.
      $fields['thumbnails'] = BaseFieldDefinition::create('image')
        ->setLabel(t('Thumbnails'))
        ->setDescription(t('The video thumbnails.'))
        ->setDisplayOptions('view', array(
          'label' => 'above',
          'type' => 'image',
          'weight' => -4,
        ))
        ->setDisplayOptions('form', array(
          'type' => 'image',
          'weight' => -4,
        ))
        ->setCardinality(-1)
        ->setDisplayConfigurable('form', TRUE)
        ->setDisplayConfigurable('view', TRUE);

    // Owner field of the transcoder job.
    // Entity reference field, holds the reference to the user object.
    // The view shows the user name field of the user.
    // The form presents a auto complete field for the user name.
    $fields['user_id'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel(t('User Name'))
      ->setDescription(t('The Name of the associated user.'))
      ->setSetting('target_type', 'user')
      ->setSetting('handler', 'default')
      ->setDisplayOptions('view', array(
        'label' => 'above',
        'type' => 'author',
        'weight' => -3,
      ))
      ->setDisplayOptions('form', array(
        'type' => 'entity_reference_autocomplete',
        'settings' => array(
          'match_operator' => 'CONTAINS',
          'size' => 60,
          'placeholder' => '',
        ),
        'weight' => -3,
      ))
      ->setDisplayConfigurable('form', TRUE)
      ->setDisplayConfigurable('view', TRUE);

    $fields['state'] = BaseFieldDefinition::create('list_string')
      ->setLabel(t('Job state'))
      ->setDescription(t('The transcoder job state.'))
      ->setSettings(array(
        'allowed_values' => array(
          'idle' => 'Idle',
          'processing' => 'Processing',
          'finished' => 'Finished'
        ),
      ))
      ->setDisplayOptions('view', array(
        'label' => 'above',
        'type' => 'string',
        'weight' => -5,
      ))
      ->setDisplayOptions('form', array(
        'type' => 'options_select',
        'weight' => -5,
      ))
      ->setDisplayConfigurable('form', FALSE)
      ->setDisplayConfigurable('view', FALSE);

    $fields['langcode'] = BaseFieldDefinition::create('language')
      ->setLabel(t('Language code'))
      ->setDescription(t('The language code of Transcode Job entity.'));

    $fields['created'] = BaseFieldDefinition::create('created')
      ->setLabel(t('Created'))
      ->setDescription(t('The time that the entity was created.'));

    $fields['changed'] = BaseFieldDefinition::create('changed')
      ->setLabel(t('Changed'))
      ->setDescription(t('The time that the entity was last edited.'));

    $fields['finished'] = BaseFieldDefinition::create('changed')
      ->setLabel(t('Finished'))
      ->setDescription(t('The time that the transcode job was finished.'));

    $fields['submitted'] = BaseFieldDefinition::create('changed')
      ->setLabel(t('Submitted'))
      ->setDescription(t('The time that the transcode job was submitted.'));

    return $fields;
  }

}
