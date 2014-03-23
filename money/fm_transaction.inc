<?php

/*
 * Helper-function to map the results of taxonomy_get_children to an
 * assoc-array, that can be handled by #options
 *
 * @param parent
 * 	the parent term-id to be loaded
 *
 * @returns
 * 	An associative array with the term-id as key '<short> (<desc>)' as value
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
 * 	The entity that will serve as basis for this purpose.
 *
 * @param type
 * 	Either 'transfer' or 'adjustment' (parsed from the URI)
 */
function fm_transaction_form($form, &$form_state, $entity, $type) {
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
		foreach	(module_implements('fm_transaction_sids') as $module)
			foreach (module_invoke($module, 'fm_transaction_sids', $user, $type) as $sid => $name)
				$sids[$sid] = $name;

		// Make sure user-accounts show up first
		natcasesort($sids);

		if (!$entity->counter)		// Don't need the sid when adjusting
			$entity->sid = 0;
		else if (count($sids) == 1 || user_access('transfer other funds'))
			$form['sid'] = array(
				'#type'		=> user_access('transfer other funds') ? 'textfield' : 'value',
				'#title'	=> t('Sender'),
				'#required'	=> true,
				'#default_value' => isset($sids[$user->uid]) ? fratler_money_format_account($user->uid) : fratler_money_format_account(key($sids)),
				'#autocomplete_path' => 'fratler/fm_autocomplete/sid/' . $type,
			);
 		else 
			$form['sid'] = array(
				'#type'		=> 'select',
				'#title'	=> t('Sender'),
				'#options'	=> $sids,
				'#required'	=> true,
				'#default_value' => isset($sids[$user->uid]) ? $user->uid : 0,	
				'#empty_option' => ' - ' . t('Select Account') . ' - ',
				'#empty_value'	=> 0,
			);

		$form['rid'] = array(
			'#type'		=> 'textfield',
			'#title'	=> $entity->counter ? t('Recipient') : t('Account'),
			'#required'	=> true,
			'#default_value' => ($entity->tid <> 0) ? _fm_format_account($entity->rid) : (!$entity->counter && !user_access('adjust other accounts') ? _fm_format_account($user->uid) : ''),	// TODO: finalize this one once dev is complete
			'#disabled'	=> (!$entity->counter && !user_access('adjust other accounts')) ? true : false,
			'#autocomplete_path' => 'fratler/fm_autocomplete/rid/' . $type,
		);

		$form['amount'] = array(
			'#type'		=> 'textfield',
			'#title'	=> t('Amount'),
			'#required'	=> true,
			'#field_prefix' => _fm_cs_psfix(true),
			'#field_suffix' => _fm_cs_psfix(false),
			'#default_value' => _fm_numerize($entity->amount),
			'#size'		=> 5,
			'#maxlenght'	=> 7,
		);

		$form['txt'] 	= array(
			'#type'		=> 'textfield',
			'#title'	=> t('Transaction-Memo'),
			'#required'	=> true,
			'#default_value' => $entity->txt,
		);

		$form['ttype'] = array(
			'#type'		=> (count(taxonomy_get_children(variable_get('fm_vocab_' . $type))) <= 1) ? 'value' : 'select',
			'#title'	=> t('Transaction-Type'),
			'#required'	=> true,
			'#options'	=> _fm_map_ttypes(variable_get('fm_vocab_' . $type)),
			'#default_value' => ($entity->ttype <> 0) ? $entity->ttype : ($type == 'transfer') ? variable_get('fm_vocab_transfer_term') : null,
			'#empty_option'	=> ' - ' . t(taxonomy_term_load(variable_get('fm_vocab_' . $type))->name) . ' - ',
		);

		$form['fratler_transaction'] = array(	// Entity-Container - no touching!
			'#type'		=> 'value',
			'#value'	=> $entity,
		);

		$form['submit'] = array(
			'#type'		=> 'submit',
			'#value'	=> t('Submit'),
			'#weight'	=> 100,		// Probably unnecessary, but hey
		);

		return $form;
	} else {
		$form['fratler_transaction'] = array(	// Entity-Container - no touching!
			'#type'		=> 'value',
			'#value'	=> $entity,
		);

		// We're gonna hijack this for a moment
		$entity->balance = $entity->counter ? fm_get_balance($entity->sid)-$entity->amount : fm_get_balance($entity->rid)+$entity->amount;

		// Draw me like one of your French tables...
		$markup = fm_display_transaction($entity, FMTransactionStyle::FM_DEFAULT ^ FMTransactionStyle::FM_TID ^ FMTransactionStyle::FM_SIGNATURE);

		// Calculate the new acount-balance
		$entity->balance = fm_get_balance($entity->rid) + $entity->amount;


		$form['markup'] = array(
			'#markup'	=> $markup,
		);
		$form['create_new'] = array(
			'#type'		=> 'checkbox',
			'#title'	=> t('Create new transaction after this one.'),
			'#default_value' => 0,
		);

		return confirm_form($form, t('Are you sure you want to proceed?'), isset($_GET['destination']) ? $_GET['destination'] : 'fratler/transaction/new/' . $type, '', t('Proceed'));

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
		$sid = fratler_money_account_load_by_name(check_plain($form_state['values']['sid']));
		if (!$sid)
			form_set_error('sid', t('@account invalid.', array('@account' => $form['sid']['#title'])));
		else
			$entity->sid = $sid->aid;
	} 
	else
		$entity->sid = 0;	// Juuuust in case...
	
	$rid = fratler_money_account_load_by_name(check_plain($form_state['values']['rid']));
	if (!$rid)
		form_set_error('rid', t('@account invalid.', array('@account' => $form['rid']['#title'])));
	else
		$entity->rid = $rid->aid;

	// If we're adjusting or someone is making a transaction in someone else's name, 
	// set the initiator-ID (iid).
	if (!$entity->counter || $entity->sid != $user->uid)
		$entity->iid = $user->uid;

	$entity->amount = _fm_parse_number(check_plain($form_state['values']['amount']));
	form_set_value($form['amount'], $entity->amount, $form_state);		// Make it look pretty to avoid confusion with decimal / thousands sep

	$entity->ttype = check_plain($form_state['values']['ttype']);

	// Vet the transaction-memo
	$entity->txt = filter_xss($form_state['values']['txt'], array('em', 'strong','cite','b','i','u'));

	// Set the timestamp
	$entity->tstamp = time();

	// Calculate the new acount-balance
	$entity->balance = fm_get_balance($entity->rid) + $entity->amount;

	// Do a dry-run on the transaction, to see, if it will go through
	foreach (module_implements('fm_transaction_dry_run') as $module) {
	       foreach (module_invoke($module, 'fm_transaction_dry_run', $entity) as $error => $field) 
			form_set_error($field, fm_get_error($error));
	}

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
	$entity = fratler_money_transaction_save($entity);

	if (!$form_state['values']['create_new'])
		$form_state['redirect'] = 'fratler/transaction/view/' . $entity->tid;
 
}

