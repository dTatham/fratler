<?php
// vim:fenc=utf-8:ft=php:ai:si:ts=2:sw=2:et:nu:fdm=indent:fdn=1:

/*
 * Helper-function to map the results of taxonomy_get_children to an
 * assoc-array, that can be handled by #options
 *
 * @param parent
 *   the parent term-id to be loaded
 *
 * @returns
 *   An associative array with the term-id as key '<short> (<desc>)' as value
 */
function _fm_map_ttypes($parent) {
  $children = taxonomy_get_children($parent);
  $bunker = array();
  foreach ($children as $child)
    $bunker[$child->tid] = $child->name . ' (' . $child->description . ')';

  return $bunker;

}

/**
 * Displays the transaction / adjustment form
 *
 * @param entity
 *   The entity that will serve as basis for this purpose.
 *
 * @param type
 *   Either 'transfer' or 'adjustment' (parsed from the URI)
 */
function fm_transaction_form($form, &$form_state, $entity, $type) {
  drupal_set_title($type == 'transfer' ? t('New transfer') : t('New adjustment'), PASS_THROUGH);
  global $user;

  // Call me paranoid...
  $type = check_plain($type);

  // Since the transfer-types are grabbed through variable_get, there're exactly 2 types of transactions we can accept.
  if (!is_object($entity) || ($type != 'transfer' && $type != 'adjustment'))
    return drupal_not_found();
  // If we're just adjusting, we need no counter-balance
  if ($type == 'adjustment')
    $entity->counter = false;

  // Display the normal form
  if(!isset($form_state['storage']['confirm'])) {
    // Grab all the accounts the user can transfer from
    $sids = array();
    foreach  (module_implements('fac_get_accounts') as $module)
      foreach (module_invoke($module, 'fac_get_accounts', $user, $type) as $account)
        $sids[$account->name] = $account->name;

    // Make sure user-accounts show up first
    natcasesort($sids);

    if (!$entity->counter)    // Don't need the sid when adjusting
      $entity->sid = 0;
    else if (count($sids) == 1 || fratler_accounts_access(FAcPerm::TRANSFER_ALL))
      $form['sid'] = array(
        '#type'    => fratler_accounts_access(FacPerm::TRANSFER_ALL) ? 'textfield' : 'value',
        '#title'  => t('Sender'),
        '#required'  => true,
        '#default_value' => $sids[format_username($user)],
        '#autocomplete_path' => 'fratler/fac_autocomplete/sid/' . $type,
      );
     else 
      $form['sid'] = array(
        '#type'    => 'select',
        '#title'  => t('Sender'),
        '#options'  => $sids,
        '#required'  => true,
        '#default_value' => isset($sids[format_username($user)]) ? format_username($user) : 0,  
        '#empty_option' => ' - ' . t('Select Account') . ' - ',
        '#empty_value'  => 0,
      );

    $form['rid'] = array(
      '#type'    => 'textfield',
      '#title'  => $entity->counter ? t('Recipient') : t('Account'),
      '#required'  => true,
      '#default_value' => ($entity->tid <> 0) ? fratler_account_load($entity->rid)->name : (!$entity->counter && !fratler_accounts_access(FAcPerm::ADJUST_OTHER) ? $sids[$user->uid]->name : ''),  // TODO: finalize this one once dev is complete
      '#disabled'  => (!$entity->counter && !fratler_accounts_access(FAcPerm::ADJUST_OTHER)) ? true : false,
      '#autocomplete_path' => 'fratler/fac_autocomplete/rid/' . $type,
    );

    $form['amount'] = array(
      '#type'    => 'textfield',
      '#title'  => t('Amount'),
      '#required'  => true,
      '#field_prefix' => _fac_cs_psfix(true),
      '#field_suffix' => _fac_cs_psfix(false),
      '#default_value' => _fac_format_number($entity->amount),
      '#size'    => 5,
      '#maxlenght'  => 7,
    );

    $form['txt']   = array(
      '#type'    => 'textfield',
      '#title'  => t('Transaction-Memo'),
      '#required'  => true,
      '#default_value' => $entity->txt,
    );

    $form['ttype'] = array(
      '#type'    => (count(taxonomy_get_children(variable_get('ft_vocab_' . $type))) <= 1) ? 'value' : 'select',
      '#title'  => t('Transaction-Type'),
      '#required'  => true,
      '#options'  => _fm_map_ttypes(variable_get('ft_vocab_' . $type)),
      '#default_value' => ($entity->ttype <> 0) ? $entity->ttype : ($type == 'transfer') ? variable_get('ft_vocab_transfer_term') : null,
      '#empty_option'  => ' - ' . t(taxonomy_term_load(variable_get('ft_vocab_' . $type))->name) . ' - ',
    );

    $form['fratler_transaction'] = array(  // Entity-Container - no touching!
      '#type'    => 'value',
      '#value'  => $entity,
    );

    $form['submit'] = array(
      '#type'    => 'submit',
      '#value'  => t('Submit'),
      '#weight'  => 100,    // Probably unnecessary, but hey
    );

    return $form;
  } 
  // Display confirm-page
  else {
    $form['fratler_transaction'] = array(  // Entity-Container - no touching!
      '#type'    => 'value',
      '#value'  => $entity,
    );

    $rid = fratler_account_load($entity->rid);
    $entity->balance_rid = $rid->balance+$entity->amount;
    if ($entity->counter) {
      $sid = fratler_account_load($entity->sid);
      $entity->balance_sid = $sid->balance-$entity->amount;
    }
    // We're gonna hijack this for a moment
#    $entity->balance = $entity->counter ? $sid->balance-$entity->amount : $rid->balance+$entity->amount;

    // Draw me like one of your French tables...
    $style = FMTransactionStyle::FM_DEFAULT ^ FMTransactionStyle::FM_TID ^ FMTransactionStyle::FM_SIGNATURE;
    if (!$entity->counter)
      $style ^= FMTransactionStyle::FM_BAL_SID;
    $markup = fratler_transaction_display($entity, $style);

    // Calculate the new acount-balance
    $entity->balance = fratler_account_get_balance($entity->rid) + $entity->amount;


    $form['markup'] = array(
      '#markup'  => $markup,
    );
    $form['create_new'] = array(
      '#type'    => 'checkbox',
      '#title'  => t('Create new transaction after this one.'),
      '#default_value' => 0,
    );

    return confirm_form($form, t('Are you sure you want to proceed?'), isset($_GET['destination']) ? $_GET['destination'] : 'fratler/transaction/' . $type, '', t('Proceed'));

  }


}

function fm_transaction_form_validate($form, &$form_state) {
  global $user;

  $entity = $form_state['values']['fratler_transaction'];
  if (!is_object($entity)) {
    form_set_error(null, t('Something just went horribly wrong. Please panic now.'));
    return;
  }

  // Since the only thing present in the confirm-page is the entity itself, we only need to run validations once.
  if (isset($form_state['storage']['confirm'])) {
    if (!is_numeric($form_state['values']['create_new']))
      form_set_error('create_new', t('Invalid value.'));
    return;
  }

  // Sender is only interesting, if we're transferring
  if ($entity->counter) {
    $sid = fratler_account_load_by_name(check_plain($form_state['values']['sid']));
    if (!$sid)
      form_set_error('sid', t('@account invalid.', array('@account' => $form['sid']['#title'])));
    else
      $entity->sid = $sid->aid;
  } 
  else
    $entity->sid = 0;  // Juuuust in case...
  
  $rid = fratler_account_load_by_name(check_plain($form_state['values']['rid']));
  if (!$rid)
    form_set_error('rid', t('@account invalid.', array('@account' => $form['rid']['#title'])));
  else
    $entity->rid = $rid->aid;

  // If we're adjusting or someone is making a transaction in someone else's name, 
  // set the initiator-ID (iid).
  if (!$entity->counter || $entity->sid != $user->uid)
    $entity->iid = $user->uid;

  $entity->amount = _fac_parse_number(check_plain($form_state['values']['amount']));
  form_set_value($form['amount'], $entity->amount, $form_state);    // Make it look pretty to avoid confusion with decimal / thousands sep

  $entity->ttype = check_plain($form_state['values']['ttype']);

  // Vet the transaction-memo
  $entity->txt = filter_xss($form_state['values']['txt'], array('em', 'strong','cite','b','i','u'));

  // Set the timestamp
  $entity->tstamp = time();

  // Calculate the new acount-balance
  $entity->balance = fratler_account_get_balance($entity->rid) + $entity->amount;

  // Do a dry-run on the transaction, to see, if it will go through
  foreach (module_implements('ft_transaction_dry_run') as $module) {
    foreach (module_invoke($module, 'ft_transaction_dry_run', $entity) as $error => $field)
      form_set_error($field, ft_get_error($error));
  }

  // Only if the dry-run was successful: greenlight the transaction.
  if (empty(form_get_errors()))
    $entity->signature_ok = TRUE;

  // Save up the entity
  form_set_value($form['fratler_transaction'], $entity, $form_state);
}

function fm_transaction_form_submit($form, &$form_state) {
  // Issue a rebuild-order to display the confirm-Page
  if (!isset($form_state['storage']['confirm'])) {
    $form_state['storage']['confirm'] = true;
    $form_state['rebuild'] = true;
    return;
  }

  $entity = $form_state['values']['fratler_transaction'];

  // TODO: check database for existing signature in case some genius hit the back-button
  $entity = fratler_transaction_save($entity);

  if (!$form_state['values']['create_new'])
    $form_state['redirect'] = 'fratler/transaction/' . $entity->tid;
 
}

