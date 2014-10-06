<?php
// vim:fenc=utf-8:ft=php:ai:si:ts=2:sw=2:et:nu:fdm=indent:fdn=1:
/**
 * @file
 * Admin-Interface and everything related
 */

/**
 * Page callback: fratler_transactions settings
 *
 * @see fratler_transactions_menu()
 */
function fratler_transactions_admin($form, &$form_state) {
  // Initialize formatter-object
  $fmt  = _fac_nf_init();

  $form['transfer'] = array(
    '#type'    => 'fieldset',
    '#title'  => t('Transaction related settings'),
  );


  // Maximum transaction-amount
  $form['transfer']['ft_trans_max'] = array(
    '#type'    => 'textfield',
    '#title'  => t('Maximum transaction amount'),
    '#description'  => t('The highest amount that can be transferred in one transaction.<br />Enter 0 for unlimited transactions.'),
    '#default_value' => _fac_format_number(variable_get('ft_trans_max')),
    '#size'    => 8,
    '#field_prefix'  => _fac_cs_psfix(true, $fmt),
    '#field_suffix' => _fac_cs_psfix(false, $fmt),
    '#attributes'   => array('class' => array('fratler_amount')),
  );

  // Overdraw-Limit
        $form['transfer']['ft_overdraw_limit'] = array(
    '#type'    => 'textfield',
    '#title'  => t('Overdraw limit'),
    '#description'  => t('The amount a normal user may overdraw its amount by.<br />Enter a positive value to keep a "safety-deposit".<br />NOTE: If you enter a positive value, the permission @permission loses it\'s effect.', array('@permission' => 'Overdraw own account')),
    '#default_value' => _fac_format_number(variable_get('ft_overdraw_limit')),
    '#size'    => 8,
    '#field_prefix'  => _fac_cs_psfix(true, $fmt),
    '#field_suffix' => _fac_cs_psfix(false, $fmt),
    '#attributes'   => array('class' => array('fratler_amount')),
  );

  // Link to rebuild the Transaction Accounts / Permissions
  $form['rebuild_perm'] = array(
    '#type'    => 'item',
    '#title'  => t('Rebuild Account table'),
    '#description'  => t('After changing !permissions, please remember to !rebuild the internal permissions table.', array(
      '!permissions' => l(t('permissions'), 'admin/people/permissions', array('fragment' => 'module-fratler_transactions', 'query' => drupal_get_destination())),
      '!rebuild' => l(t('rebuild'), 'admin/config/fratler/accounts/recreate_accounts', array('query' => drupal_get_destination())),)),
  );


  return system_settings_form($form);
}

/**
 * implements form_validate()
 */
function fratler_transactions_admin_validate($form, &$form_state) {
  // Work the usual magic...
  if (!is_numeric(_fac_parse_number($form_state['values']['ft_trans_max'])) || _fac_parse_number($form_state['values']['ft_trans_max']) < 0)
    form_set_error('ft_trans_max', t('Invalid maximum transaction amount'));
       if (!is_numeric(_fac_parse_number($form_state['values']['ft_overdraw_limit'])))
           form_set_error('ft_overdraw_limit', t('Invalid overdraw amount'));
  else  // Convert amount
    form_set_value($form['transfer']['ft_overdraw_limit'], _fac_parse_number($form_state['values']['ft_overdraw_limit']), $form_state);

  // Convert transaction-amount
  form_set_value($form['transfer']['ft_trans_max'], _fac_parse_number($form_state['values']['ft_trans_max']), $form_state);
}

/**
 * Page callback to display and administer Transaction Types
 */
function ft_ttypes_admin() {

  return array();
}
