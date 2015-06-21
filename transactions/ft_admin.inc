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

  $rows = array();
  foreach(fratler_transaction_type_load_multiple() as $ttype) {
    $rows[$ttype->category][$ttype->ttid] = array(
      $ttype->locked ? '&#x1F512;' : '',
      $ttype->name,
      $ttype->description,
      $ttype->selectable ? '&#x2714;' : '&#x2718;',
      $ttype->active ? '&#x2714;' : '&#x2718;',
      t('!edit !delete',
        array('!edit' => $ttype->selectable ? l('Edit', 'admin/config/fratler/transactions/ttypes/' . $ttype->ttid . '/edit') : '---',
              '!delete' => $ttype->locked ? '' : l('Delete', 'admin/config/fratler/transactions/ttypes/' . $ttype->ttid . '/delete'),
            )),
    );
  }

  return array(
    'sec_transfers' => array(
      '#theme'  => 'table',
      '#header' => array('', t('Name'), t('Description'), t('Selectable'), t('Active'), t('Operations')), 
      '#rows'   => $rows['transfer'],
      '#caption' => t('Transfers'),
      '#sticky' => TRUE,
    ),

    'sec_deposits' => array(
      '#theme'  => 'table',
      '#header' => array('', t('Name'), t('Description'), t('Selectable'), t('Active'), t('Operations')), 
      '#rows'   => $rows['deposit'],
      '#caption' => t('Deposits'),
      '#sticky' => TRUE,
    ),

    'sec_withdrawal' => array(
      '#theme'  => 'table',
      '#header' => array('', t('Name'), t('Description'), t('Selectable'), t('Active'), t('Operations')), 
      '#rows'   => $rows['withdrawal'],
      '#caption' => t('Withdrawals'),
      '#sticky' => TRUE,
    ),

  );
}

/**
 * Form callback to add/edit Transaction Type.
 */
function ft_ttypes_edit($form, &$form_state, $ttype) {
  // In case we're creating a new Transaction Type, set it to be always selectable (only automated transactions are non-selectable).
  if ($ttype->ttid == 0)
    $ttype->selectable = 1;

  $form['ttype'] = array(
    '#type' => 'value',
    '#value' => $ttype,
  );

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('Please assign a transaction-code (up to three characters).'),
    '#required' => TRUE,
    '#size' => 8,
    '#maxlength' => 8,
    '#default_value' => $ttype->name,
  );

  $form['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Description'),
    '#description' => t('Please give a short description of this Transaction Type.'),
    '#default_value' => $ttype->description,
    '#required' => TRUE,
  );

  $form['category'] = array(
    '#type' => 'select',
    '#title' => t('Transaction category'),
    '#options' => array(
      'transfer' => t('Transfer'),
      'deposit' => t('Deposit'),
      'withdrawal' => t('Withdrawal'),
    ),
    '#required' => TRUE,
    '#disabled' => $ttype->locked,
  );

  if ($ttype->ttid)
    $form['category']['#default_value'] = $ttype->category;

  $form['active'] = array(
    '#type' => 'checkbox',
    '#title' => t('Active'),
    '#default_value' => $ttype->active,
    '#disabled' => $ttype->locked,
  );

  $form['actions'] = array(
    '#type' => 'actions',

    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    ),
    'cancel' => array(
      '#markup' => l(t('Cancel'), 'admin/config/fratler/transactions/ttypes'),
    ),
  );

  return $form;
}

/**
 * Validate Transaction Type 
 */
function ft_ttypes_edit_validate($form, &$form_state) {
  $ttype = $form_state['values']['ttype'];

  if (!$ttype->ttid && fratler_transaction_type_load_by_name($form_state['values']['name']))
    form_set_error('name', t('This name is already taken, please choose another one.'));
}

/**
 * Submit Transaction Type
 */
function ft_ttypes_edit_submit($form, &$form_state) {
  $ttype = $form_state['values']['ttype'];
  $ttype->name = check_plain($form_state['values']['name']);
  $ttype->description = check_plain($form_state['values']['description']);
  $ttype->category = check_plain($form_state['values']['category']);
  $ttype->active = check_plain($form_state['values']['active']);

  $ttype = fratler_transaction_type_save($ttype);

  $form_state['redirect'] = 'admin/config/fratler/transactions/ttypes';
}

/**
 * Form callback to delete Transaction Type.
 */
function ft_ttypes_delete($form, &$form_state, $ttype) {
  if ($ttype->locked) {
    drupal_set_message(t('This Transaction Type is locked, you cannot delete it.'), 'error');
    drupal_goto('admin/config/fratler/transactions/ttypes');
  }

  // Check if this type has transactions attached to it.
  $deadlock = db_select('fratler_transactions', 'f')
    ->fields('f', array('tid'))
    ->condition('ttype', $ttype->ttid)
    ->countQuery()
    ->execute()
    ->fetchField();

  if ($deadlock) {
    drupal_set_message(format_plural($deadlock, 'There is currently <b>one</b> transaction of this type. You need to rebook or cancel this transaction first in order to delete this type.',
      'There are currently <b>@count</b> transactions of this type. You need to rebook or cancel these transactions first in order to delete this type.'), 'error');
    drupal_goto('admin/config/fratler/transactions/ttypes');
  }

  $form['conf'] = array(
    '#type' => 'fieldset',
    '#title' => t('Confirm deletion'),
    '#description' => t('Please confirm you want to delete this Transaction Type. <b>This action cannot be undone.</b>'),
    '#collapsible' => FALSE,
  );

  $form['conf']['ttype'] = array(
    '#type' => 'value',
    '#value' => $ttype,
  );

  $form['conf']['name'] = array(
    '#type' => 'item',
    '#title' => t('Transaction Type'),
    '#markup' => $ttype->name . ' (' . $ttype->description . ')',
  );

  $form['conf']['yup'] = array(
    '#type' => 'checkbox',
    '#title' => t('I know what I am doing.'),
    '#default_value' => FALSE,
  );

  $form['actions'] = array(
    '#type' => 'actions',

    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    ),
    'cancel' => array(
      '#markup' => l(t('Cancel'), 'admin/config/fratler/transactions/ttypes'),
    ),
  );

  return $form;
}

function ft_ttypes_delete_validate($form, &$form_state) {
  if (!$form_state['values']['yup'])
    form_set_error('yup', t('Please confirm you know what you are about to do.'));
}

function ft_ttypes_delete_submit($form, &$form_state) {
  fratler_transaction_type_delete($form_state['values']['ttype']);
  $form_state['redirect'] = 'admin/config/fratler/transactions/ttypes';
}
