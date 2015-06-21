<?php
// vim:fenc=utf-8:ft=php:ai:si:ts=2:sw=2:et:nu:fdm=indent:fdn=1:

/*
 * Helper-function to map the ttypes in the proper form.
 *
 * @param ttypes
 *   An array of ttype-entities.
 *
 * @returns
 *   An associative array with the term-id as key and '<short> (<desc>)' as value.
 */
function _ft_map_ttypes($ttypes) {
  $mapped = array();

  foreach ($ttypes as $ttype)
    $mapped[$ttype->ttid] = t('@name (@description)', array('@name' => t($ttype->name), '@description' => t($ttype->description)));

  asort($mapped);

  return $mapped;

}

/**
 * Displays the transaction / adjustment form
 *
 * @param transaction
 *   The entity that will serve as basis for this purpose.
 *
 * @param type
 *   Either 'transfer' or 'adjustment' (parsed from the URI)
 */
function fm_transaction_form($form, &$form_state, $transaction, $type) {
  global $user;

  drupal_set_title($type == 'transfer' ? t('New transfer') : t('New adjustment'), PASS_THROUGH);

  // Call me paranoid...
  $type = check_plain($type);

  // Since the transfer-types are grabbed through variable_get, there're exactly 2 types of transactions we can accept.
  if (!is_object($transaction) || ($type != 'transfer' && $type != 'adjustment'))
    return drupal_not_found();

  // In case we are in step 2 (verification), we need to retrieve the existing transaction-entity:
  if (isset($form_state['values']['fratler_transaction']))
    $transaction = &$form_state['values']['fratler_transaction'];

  else {
    // Snap the transaction-entity into the form to prevent fiddling around with data in the 2nd step.
    $form['fratler_transaction'] = array(  // Entity-Container - no touching!
      '#type'    => 'value',
      '#value'  => &$transaction,
    );
    // If we're just adjusting, we need no counter-balance
    if ($type == 'adjustment')
      $transaction->counter = false;
  }

  // Here we go.
  
  // Sender-Id (sid):
  // By default, this is the current user. However, some users may either transfer from other accounts,
  // or other modules (Master Accounts) may grant them permission to transfer from them.
  //
  // This is only relevant if we are transferring, adjustments have a sid of 0.
  if (!$transaction->counter)
    $transaction->sid = 0;

  else {
    // Grab all the accounts the user can transfer from:
    $sids = array();
    foreach  (module_implements('fac_get_accounts') as $module)
      foreach (module_invoke($module, 'fac_get_accounts', $user, $type) as $account)
        $sids[$account->name] = $account->name;

    // Make sure user-accounts show up first
    natcasesort($sids);

    $form['tdata']['sid'] = array(
      '#type' => fratler_accounts_access(FAcPerm::TRANSFER_ALL) ? 'textfield' : (count($sids) > 1 ? 'select' : 'value'),
      '#title' => t('Sender'),
      '#required' => TRUE,
      // TODO: Let's see how well this plays with usernames that are changed by format_username().
      '#default_value' => isset($sids[format_username($user)]) ? format_username($user) : '', 
      '#autocomplete_path' => 'fratler/fac_autocomplete/sid/' . $type,

      '#empty_option' => t('- Select Account -'),
      '#empty_value'  => 0,
      '#options' => $sids,
    );
  }

  // Recipient-Id (rid):
  // This is quite straight-forward. Every active account with the appropriate permission can receive transactions.
  // In case we adjust, we throw in a quick check what the current user may do and fill in fields accordingly.
  $form['tdata']['rid'] = array(
    '#type' => 'textfield',
    '#title'  => $transaction->counter ? t('Recipient') : t('Account', array(), array('context' => 'accounting')),
    '#required' => TRUE,
    '#default_value' => ($transaction->rid <> 0) ? fratler_account_load($transaction->rid)->name : '',
    '#autocomplete_path' => 'fratler/fac_autocomplete/rid/' . $type,
    '#disabled'  => !$transaction->counter && !fratler_accounts_access(FAcPerm::ADJUST_OTHER),

  );

  // Amount:
  // Well, guess what this is?!
  $form['tdata']['amount'] = array(
    '#type'    => 'textfield',
    '#title'  => t('Amount'),
    '#required'  => TRUE,
    '#field_prefix' => _fac_cs_psfix(TRUE),
    '#field_suffix' => _fac_cs_psfix(FALSE),
    '#default_value' => _fac_format_number($transaction->amount),
    '#size'    => 5,
    '#maxlength'  => 7,
  );

  // Text (txt):
  // Let's leave a friendly memo with the transaction.
  $form['tdata']['txt']   = array(
    '#type'    => 'textfield',
    '#title'  => t('Transaction-Memo'),
    '#required'  => TRUE,
    '#default_value' => $transaction->txt,
  );

  // Display action buttons for first round.
  if (!isset($form_state['storage']['confirm'])) {
    // Action-Buttons for first round.
    $form['actions'] = array(
      '#type' => 'actions',
      'submit' => array(
        '#type' => 'submit',
        '#value' => t('Check'),
        '#limit_validation_errors' => array(array('tdata')),
      ),
      'cancel' => array(
        '#markup' => l(t('Cancel'), '/fratler/account'),
      ),
    );

  return $form;
  }

  // Display confirm page.
  else {
    foreach (array_keys($form['tdata']) as $element) {
      $form['tdata'][$element]['#type'] = 'item';
      $form['tdata'][$element]['#markup'] = $form['tdata'][$element]['#default_value'];
    }

    $ttypes = array();
    $form['ttype'] = array(
      '#title'  => t('Transaction-Type'),
      '#required'  => true,
    );

    if ($type == 'transfer') {
      $form['ttype']['#empty_option']  = t('- Transfers -');
      $form['ttype']['#default_value'] = variable_get('ft_default_transfer');
      $ttypes = fratler_transaction_type_load_by_category($type, TRUE);
    }
    else {
      if ($transaction->amount > 0) {
        $form['ttype']['#empty_option'] = t('- Deposits -');
        $form['ttype']['#default_value'] = variable_get('ft_default_deposit');
        $ttypes = fratler_transaction_type_load_by_category('deposit', TRUE);
      }
      else {
        $form['ttype']['#empty_option'] = t('- Withdrawals -');
        $form['ttype']['#default_value'] = variable_get('ft_default_withdrawal');
        $ttypes = fratler_transaction_type_load_by_category('withdrawal', TRUE);
      }
    }

    $form['ttype']['#type'] = (count($ttypes) <= 1) ? 'value' : 'select';
    $form['ttype']['#options'] = _ft_map_ttypes($ttypes);

    $form['create_new'] = array(
      '#type'    => 'checkbox',
      '#title'  => t('Create new transaction after this one.'),
      '#default_value' => 0,
    );

    return confirm_form($form, t('Are you sure you want to proceed?'), 'fratler/transaction/' . $type, format_plural(count($ttypes), 'Please verify your input.', 'Please verify your input and select a transaction-type.'), t('Proceed'));
  }


}

function fm_transaction_form_validate($form, &$form_state) {
  global $user;

  // Retrieve the transaction.
  $transaction = &$form_state['values']['fratler_transaction'];

  // We need to do this only once for the original data.
  if (isset($form_state['storage']['confirm'])) {
    $transaction->ttype = check_plain($form_state['values']['ttype']);
    form_set_value($form['fratler_transaction'], $transaction, $form_state);

    foreach (module_implements('ft_transaction_dry_run') as $module) {
      foreach (module_invoke($module, 'ft_transaction_dry_run', $transaction) as $error => $field)
        form_set_error($field, ft_get_error($error));
    }

    // Only if the dry-run was successful: greenlight the transaction.
    if (empty(form_get_errors()))
      $transaction->signature_ok = TRUE;

    return;
  }

  // Fetch the corresponding account-ids.
  if (isset($form_state['values']['sid']))
    $transaction->sid = fratler_account_fetch_by_name(check_plain($form_state['values']['sid']));
  $transaction->rid = fratler_account_fetch_by_name(check_plain($form_state['values']['rid']));

  // Parse the amount.
  // Make it look pretty to avoid confusion with decimal / thousands sep
  $transaction->amount = _fac_parse_number(check_plain($form_state['values']['amount']));
  form_set_value($form['tdata']['amount'], _fac_format_number($transaction->amount), $form_state);

<<<<<<< HEAD
  // Run sanity checks on the amount.
  $entity->amount = _fac_parse_number(check_plain($form_state['values']['amount']));
  if (empty($entity->amount)) {
    form_set_error('amount', t('Amount invalid.'));
    return;
  }

=======
  // Vet the transaction-memo.
  $transaction->txt = filter_xss($form_state['values']['txt'], array('em', 'strong','cite','b','i','u'));
>>>>>>> devel

  // Internal stuff:
  // If this transaction is only initiated by the user, set the initiator-id (iid):
  if ($transaction->sid != $user->uid)
    $transaction->iid = $user->uid;

  // Set the timestamp.
  $transaction->tstamp = time();

  // Calculate the new account balance:
  $transaction->balance = fratler_account_get_balance($transaction->rid) + $transaction->amount;

  form_set_value($form['fratler_transaction'], $transaction, $form_state);

  foreach (module_implements('ft_transaction_dry_run') as $module) {
    foreach (module_invoke($module, 'ft_transaction_dry_run', $transaction) as $error => $field)
      form_set_error($field, ft_get_error($error));
  }

  if (empty(form_get_errors())) {
    $form_state['rebuild'] = TRUE;
    $form_state['storage']['confirm'] = TRUE;
  }

  return;
}

function fm_transaction_form_submit($form, &$form_state) {
  $transaction = $form_state['values']['fratler_transaction'];

  // TODO: check database for existing signature in case some genius hit the back-button

  $transaction = fratler_transaction_save($transaction);

  if (!$form_state['values']['create_new']) {
    $form_state['redirect'] = 'fratler/transaction/' . $transaction->tid;
  }
  else
    drupal_set_message(t('Transaction completed successfully.'));
}

