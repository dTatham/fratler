<?php
// vim:fenc=utf-8:ft=php:ai:si:ts=2:sw=2:et:nu:fdm=indent:fdn=1:
/**
 * @file
 * Admin interface and everything related
 */

/**
 * Menu callback: toggle active/inactive
 *
 * @param account
 *   The fully-loaded Master Account.
 */
function fratler_masters_admin_toggle($account) {
  $account->active = !$account->active;

  fratler_masters_save($account);

  if (drupal_get_destination() != array('destination' => current_path()))
    drupal_goto(drupal_get_destination());
  else
    drupal_goto('admin/config/fratler/master');
}
/**
 * Page callback to display Master Account administration.
 */
function fratler_masters_admin($form, &$form_state) {
  $header = array(
    'name'    => array('data' => t('Name'), 'type' => 'property', 'specifier' => 'name', 'sort' => 'asc'),
    'active'  => array('data' => t('Active'), 'type' => 'property', 'specifier' => 'active'),
    'description'  => array('data' => t('Description'), 'type' => 'property', 'specifier' => 'description'),
    'operations'  => array('data' => t('Operations'),),
  );

  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'fratler_masters')
    ->tableSort($header);

  $rows = array();
  $results = $query->execute();

  if ($results) {
    foreach (fratler_masters_load_multiple(array_keys($results['fratler_masters'])) as $master) {
      $rows[$master->mid] = array(
        'name'    => l($master->name, 'admin/config/fratler/master/' . -$master->mid),
        'active'  => $master->active ? '&#x2714' : '&#x2718',
        'description'  => $master->description,
        'operations'  => t('!view !statement !edit !toggle !permissions', array(
              '!view'   => l(t('view'), 'admin/config/fratler/master/' . -$master->mid),
              '!statement'  => l(t('statement'), 'fratler/statement/' . $master->mid),
              '!edit'   => l(t('edit'), 'admin/config/fratler/master/' . -$master->mid . '/edit'),
              '!toggle'   => l(t('toggle'), 'admin/config/fratler/master/' . -$master->mid . '/toggle', array('query' => drupal_get_destination())),
              '!permissions'  => l(t('permissions'), 'admin/people/permissions', array('fragment' => 'module-fratler_masters', 'query' => drupal_get_destination())),
            )),
      );
    }
  }

  $form['fma_list'] = array(
    '#type'    => 'tableselect',
    '#header'  => $header,
    '#options'  => $rows,
    '#empty'  => t('No Master Accounts available.'),
  );
  return $form;
}

/**
 * Page callback to display Master Account edit/create form
 */
function fratler_masters_admin_edit($form, &$form_state, $account) {
  $form['fma_name'] = array(
    '#type'    => 'textfield',
    '#title'  => t('Name'),
    '#required'  => true,
    '#default_value' => $account->name,
  );
  $form['mach_name'] = array(
    '#type'    => 'machine_name',
    '#default_value' => $account->mach_name,
    '#machine_name'  => array(
      'exists'  => '_fma_check_mach_name',
      'source'  => array('fma_name'),
    ),
  );

  $form['active'] = array(
    '#type'    => 'checkbox',
    '#title'  => t('Active?'),
    '#description'  => t('Choose whether this Master Account is to be active or inactive.'),
    '#default_value' => $account->active,
  );
  $form['description'] = array(
    '#type'    => 'textfield',
    '#title'  => t('Description'),
    '#description'  => t('Please give a short description of the Master Account.'),
    '#default_value' => $account->description,
  );
  $form['actions'] = array('#type' => 'actions',);

  $form['actions']['submit'] = array(
    '#type'    => 'submit',
    '#value'  => t('Save'),
  );
  if ($account->mid)
    $form['actions']['delete'] = array(
      '#type'    => 'submit',
      '#value'  => t('Delete'),
    );
  $form['actions']['cancel'] = array(
    '#markup'  => l(t('Cancel'), 'admin/config/fratler/master'),
  );

  $form['account'] = array(
    '#type'    => 'value',
    '#value'  => $account,
  );

  return $form;
}

function fratler_masters_admin_edit_validate($form, &$form_state) {
  $candidate = fratler_masters_load_by_name(check_plain($form_state['values']['fma_name']));
  $master = &$form_state['values']['account'];

  // Segway to delete-form in case someone hit a button...
  if ($master->mid != 0 && $form_state['values']['op'] == $form_state['values']['delete']) {
    drupal_goto('admin/config/fratler/master/' . -$master->mid . '/delete');
    return;
  }

  if ($master->mid == 0 && $candidate)
    form_set_error('fma_name', 
      t('This Master Account already exists. If you want to edit it, please click !here.', 
        array('!here' => l(t('here'), 'admin/config/fratler/master/' . -$candidate->mid . '/edit'))));

}

function fratler_masters_admin_edit_submit($form, &$form_state) {
  $raw     = &$form_state['values']['account'];
  $raw->name   = check_plain($form_state['values']['fma_name']);
  $raw->active   = check_plain($form_state['values']['active']);
  $raw->description = filter_xss($form_state['values']['description'], array('em', 'strong','cite','b','i','u'));
  $raw->mach_name  = check_plain($form_state['values']['mach_name']);

  // Sine we're creating a new Master Account, we can green light it:
  if ($raw->mid == 0)
    $raw->signature_ok = TRUE;

  $master = fratler_masters_save($raw);

  $form_state['redirect'] = 'admin/config/fratler/master';
}

function fratler_masters_admin_delete($form, &$form_state, $account) {
  drupal_set_title(t('Delete Master Account @name', array('@name' => $account->name)));
  $options = array();

  // Check if this account has any transactions attached to it
  $query = db_select('fratler_transactions', 'f')
    ->fields('f', array('tid'))
    ->condition(db_or()->condition('rid', $account->mid)->condition('sid', $account->mid))
    ->execute();

  $tids = $query->fetchCol('tid');

  // If there are any transactions on this account, load a list of alternative MAs these transactions can be transferred to
  if (count($tids)) {
    $query = new EntityFieldQuery;
    $query->entityCondition('entity_type', 'fratler_masters')
      ->propertyCondition('mid', -$account->mid, '<>')
      ->propertyCondition('active', 1);
    $results = $query->execute();

    // If no other active accounts are available, complain and return to admin-page
    if (!$results) {
      drupal_set_message(format_plural(count($tids), 'No alternative Master Accounts available. You need another active account that can take over the existing transaction.',
                  'No alternative Master Accounts available. You need another active account that can take over the existing @count transactions.'), 
                  'error');
      drupal_goto('admin/config/fratler/master');
    }

    $alt_accounts = fratler_masters_load_multiple(array_keys($results['fratler_masters']));

    foreach ($alt_accounts as $alt_account)
      $options[$alt_account->mid] = $alt_account->name;

  }

  $form['cont'] = array(
    '#type'    => 'fieldset',
    '#title'  => t('Confirm deletion'),
    '#description'  => t('Please confirm you want to delete this Master Account. <b>This action cannot be undone.</b>'),
    '#collapsible'  => false,
  );
  if (!$form_state['rebuild']) {
    if (count($tids)) {
      $form['cont']['alt_acc'] = array(
        '#type'    => 'select',
        '#title'  => t('Alternative Master Account'),
        '#description'  => format_plural(count($tids), 'There is currently <b>one</b> transaction on this Master Account. Please select an alternate Master Account which will hold the transaction.',
                    'There are currently <b>@count</b> transactions on this Master Account. Please select an alternate Master Account which will hold the transactions.'),
        '#options'  => $options,
        '#required'  => true,
      );
    }
    $form['cont']['confirm_merge'] = array(
      '#type'    => 'checkbox',
      '#title'  => t('I know what I am doing'),
      '#default_value' => false,
    );
  }

  // This way we avoid drupal pre-checking it when rebuilding
  else
    $form['cont']['confirm_del'] = array(
      '#type'    => 'checkbox',
      '#title'  => t('I know what I am doing'),
      '#default_value' => false,
    );

  $form['account'] = array(
    '#type'    => 'value',
    '#value'  => $account,
  );
  $form['tids'] = array(
    '#type'    => 'value',
    '#value'  => $tids,
  );
  $form['cont']['actions'] = array(
    '#type'    => 'actions',
  );
  $form['cont']['actions']['submit'] = array(
    '#type'    => 'submit',
    '#value'  => t('Confirm'),
  );
  $form['cont']['actions']['cancel'] = array(
    '#markup'  => l(t('Cancel'), 'admin/config/fratler/master'),
  );
  return $form;
}
function fratler_masters_admin_delete_validate($form, &$form_state) {
  if ((isset($form_state['values']['confirm_del']) && !$form_state['values']['confirm_del']) || 
    (isset($form_state['values']['confirm_merge']) && !$form_state['values']['confirm_merge']))
    form_set_error('confirm', t('Please confirm that you know what you are doing.'));
}

function fratler_masters_admin_delete_submit($form, &$form_state) {
  $account   = $form_state['values']['account'];
  $form_state['redirect'] = 'admin/config/fratler/master';

  // Ok, before we can do shit, we have to move shit.
  if ($form_state['values']['tids']) {
    if (fratler_transactions_reassign(fratler_account_load($account->mid), fratler_account_load($form_state['values']['alt_acc'])))
      $form_state['rebuild'] = true;
    else
      drupal_set_message(t('Operation could not be completed.'), 'error');
    return;
  }  
  else 
    fratler_masters_delete($account);

}

