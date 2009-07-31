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
 * Field handler to display the video preview thumbnail
 *
 * @ingroup views_field_handlers
 */
class video_views_handler_field_image extends views_handler_field {
    /**
   * Define options available for this field.
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['img_type'] = array('default' => 'thumbnail');
    $options['disp_link'] = array('default' => TRUE);
    return $options;
  }

  /**
   * Build option configuration form.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['img_type'] = array(
      '#title' => t('Show image as'),
      '#type' => 'select',
      '#options' => array(
        'thumbnail' => t('Thumbnail'),
        'preview' => t('Preview'),
        ),
      '#default_value' => $this->options['img_type'],
    );
    $form['disp_link'] = array(
      '#title' => t('Link image to video'),
      '#type' => 'checkbox',
      '#default_value' => $this->options['disp_link'],
    );

  }
/**
   * Render field output to the browser.
   */
  function render($values) {
    return _video_views_handler_field_image($values, $this->options['img_type'], $this->options['disp_link']);
  }
}
/**
* Handler to render the preview image associated with a video
**/
function _video_views_handler_field_image($values, $image_type, $linked) {
 if($values->node_type &&  $values->node_type != 'video') return NULL;
 $node = node_load($values->nid);
 $output = NULL;
 if($node->iid && $image = node_load($node->iid)) {
     $image_html = NULL;
     if($image != NULL && $image->type == 'image') {
        $image_html = image_display($image, $image_type, array('class' => 'video_image_teaser'));
        //Create a link with an image in it.
        $output .= ($linked ? l($image_html, "node/$values->nid", array('html' => TRUE)) : $image_html);
        $output .= '<br class="video_image_clear" />';
      }
 }
 return $output;
}
