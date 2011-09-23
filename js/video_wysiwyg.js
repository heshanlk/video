(function ($) {
  Drupal.behaviors.video_wysiwyg = {
    attach: function (context, settings) {
      $('.submit-button').click(function(){
        alert('oki');
      });
      $('.select-list li').click(function(){
        alert('works clicks');
      });
    }
  }
  
})(jQuery);