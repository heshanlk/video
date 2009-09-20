<?php
//$Id$
/**
* Implementation of hook_views_tables
*
* @return
*   array - Enables support in the video module for views integration
 * @author Glen Marianko Twitter@demoforum <glenm at demoforum dot com>
 * @todo
**/

/**
 * Field handler to display the play length of the video.
 *
 * @ingroup views_field_handlers
 */
class video_views_handler_field_playtime_seconds extends views_handler_field {
  /**
   * Define options available for this field.
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['time_type'] = array('default' => 'hms');
    return $options;
  }

  /**
   * Build option configuration form.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['time_type'] = array(
      '#title' => t('Show playtime as'),
      '#type' => 'select',
      '#options' => array(
        'hms' => t('Hour:min:sec'),
        'sec' => t('Seconds'),
        ),
      '#default_value' => $this->options['time_type'],
    );
  }
  /**
   * Render field output to the browser.
   */
  function render($values) {
    return _video_playtime_seconds($values, $this->options['time_type']);
  }
}
/**
* Handler to to render the correct playtime for the video in a field
**/
function _video_playtime_seconds($values, $type) {
  if($values->node_type &&  $values->node_type != 'video') return NULL;
  switch ($type) {
      case 'hms':
          $hms = _video_sec2hms($values->video_playtime_seconds);

          // Pad the minutes / seconds with a leading "0", if
          // necessary
          if ($hms['hours'] > 0) {
            $hms['minutes'] = str_pad($hms['minutes'], 2, '0', STR_PAD_LEFT);
          }
          $hms['seconds'] = str_pad($hms['seconds'], 2, '0', STR_PAD_LEFT);

          $out = '';
          if ($hms['hours'] > 0) {
         $out .= $hms['hours'].":";
          }
          $out .= $hms['minutes'].":".$hms['seconds'];
          return $out;
     case 'sec':
     default:
        return $values->video_playtime_seconds;
  }
}
