<?php
/* vim: set filetype=php expandtab tabstop=2 shiftwidth=2 autoindent smartindent: */
/**
 * @file
 * Admin-Interface and everything related
 */

/**
 * Page callback: fratler_money settings
 *
 * @see fratler_accounts_menu()
 */
function fratler_accounts_admin($form, &$form_state) {
  error_log("boo!!");
  // Initialize formatter-object
  $fmt  = _fac_nf_init();

  $form['general']   = array(
    '#type'    => 'fieldset',
    '#title'  => t('General Settings'),
  );

  // We'll use the "intl" Framework PHP ships with, which allows for better flexibility and it doesn't give a flying f... about installed locales
  $form['general']['fac_locale'] = array(
    '#type'    => 'textfield',
    '#title'  => t('Used locale'),
    '#default_value' => variable_get('fac_locale'),
    '#size'    => 10,
    '#maxlength'  => 30,
    '#description'  => t('Please enter a locale-String you would like to use to format the amounts (e.g. en_US or de_DE).<br />Enter "C" as fallback.'),
    '#required'  => true,
    '#field_suffix'  => t('Currently using "@locale" (example: @num)', array('@locale' => variable_get('fac_locale', 'C'), '@num' => _fac_format_amount(-1234567.890, $fmt))),
  );

  // Pager-Settings (how many entries per page)
  $form['general']['fac_pager'] = array(
    '#type'    => 'textfield',
    '#title'  => t('Statement entries per page'),
    '#description'  => t('How many statement records would you like to display per page?<br />Enter "0" to disable pagination.'),
    '#size'    => 5,
    '#maxlength'  => 3,
    '#default_value' => variable_get('fac_pager'),
  );

  // Link to rebuild the Transaction Accounts / Permissions
  $form['general']['rebuild_perm'] = array(
    '#type'    => 'item',
    '#title'  => t('Rebuild Account table'),
    '#description'  => t('After changing !permissions, please remember to !rebuild the internal permissions table.', array(
      '!permissions' => l(t('permissions'), 'admin/people/permissions', array('fragment' => 'module-fratler_accounts', 'query' => drupal_get_destination())),
      '!rebuild' => l(t('rebuild'), 'admin/config/fratler/accounts/recreate_accounts', array('query' => drupal_get_destination())),)),
  );

  return system_settings_form($form);
}

/**
 * implements form_validate()
 */
function fratler_accounts_admin_validate($form, &$form_state) {
  // Work the usual magic...
  if (!_fac_nf_init(NumberFormatter::IGNORE, $form_state['values']['fac_locale']))
    form_set_error('fac_locale', t('Invalid locale.'));
  if (!is_numeric($form_state['values']['fac_pager']) || $form_state['values']['fac_pager'] < 0)
    form_set_error('fac_pager', t('Invalid entries per page setting'));
}
