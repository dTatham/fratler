<?php
/**
 * @file
 * Admin interface and everything related
 */

/**
 * Page callback to display Master Account administration.
 */
function fratler_master_admin($form, &$form_state) {
	$header = array(
		'name'		=> array('data' => t('Name'), 'type' => 'property', 'specifier' => 'name', 'sort' => 'asc'),
		'active'	=> array('data' => t('Active'), 'type' => 'property', 'specifier' => 'active'),
		'description'	=> array('data' => t('Description'), 'type' => 'property', 'specifier' => 'description'),
		'operations'	=> array('data' => t('Operations'),),
	);

	$query = new EntityFieldQuery();
	$query->entityCondition('entity_type', 'fratler_master')
		->tableSort($header);

	$rows = array();
	$results = $query->execute();

	if ($results) {
		foreach (fratler_master_load_multiple(array_keys($results['fratler_master'])) as $master) {
			$rows[$master->mid] = array(
				'name'		=> l($master->name, 'admin/config/fratler/master/' . -$master->mid),
				'active'	=> $master->active ? '&#x2714' : '&#x2718',
				'description'	=> $master->description,
				'operations'	=> array('data' => array(
					'#type'		=> 'link',
					'#title' 	=> t('edit'),
					'#href'		=> 'admin/config/fratler/master/' . -$master->mid . '/edit',
				)),
			);
		}
	}

	$form['fma_list'] = array(
		'#type'		=> 'tableselect',
		'#header'	=> $header,
		'#options'	=> $rows,
		'#empty'	=> t('No Master Accounts available.'),
	);
	return $form;
}

/**
 * Page callback to display Master Account edit/create form
 */
function fratler_master_admin_edit($form, &$form_state, $account) {
	$form['fma_name'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Name'),
		'#required'	=> true,
		'#default_value' => $account->name,
	);
	$form['active'] = array(
		'#type'		=> 'checkbox',
		'#title'	=> t('Active?'),
		'#description'	=> t('Choose whether this Master Account is to be active or inactive.'),
		'#default_value' => $account->active,
	);
	$form['description'] = array(
		'#type'		=> 'textfield',
		'#title'	=> t('Description'),
		'#description'	=> t('Please give a short description of the Master Account.'),
		'#default_value' => $account->description,
	);
	$form['actions'] = array('#type' => 'actions',);

	$form['actions']['submit'] = array(
		'#type'		=> 'submit',
		'#value'	=> t('Save'),
	);
	if ($account->mid)
		$form['actions']['delete'] = array(
			'#type'		=> 'submit',
			'#value'	=> t('Delete'),
		);
	$form['actions']['cancel'] = array(
		'#markup'	=> l(t('Cancel'), 'admin/config/fratler/master'),
	);

	$form['account'] = array(
		'#type'		=> 'value',
		'#value'	=> $account,
	);

	return $form;
}

function fratler_master_admin_edit_validate($form, &$form_state) {
	$candidate = fratler_master_load_by_name(check_plain($form_state['values']['fma_name']));
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

function fratler_master_admin_edit_submit($form, &$form_state) {
	$raw 		= &$form_state['values']['account'];
	$raw->name 	= check_plain($form_state['values']['fma_name']);
	$raw->active 	= check_plain($form_state['values']['active']);
	$raw->description = filter_xss($form_state['values']['description'], array('em', 'strong','cite','b','i','u'));

	$master = fratler_master_save($raw);
	drupal_set_message(t('Master Account successfully @action.', array('@action' => ($raw->mid == 0) ? t('created') : t('edited'))), 'status');

	$form_state['redirect'] = 'admin/config/fratler/master';
}

function fratler_master_admin_delete($form, &$form_state, $account) {
	drupal_set_title(t('Delete Master Account @name', array('@name' => $account->name)));
	$options = array();

	// Check if this account has any transactions attached to it
	$query = db_select('fratler_money_currents', 'f')
		->fields('f', array('tid'))
		->condition(db_or()->condition('rid', $account->mid)->condition('sid', $account->mid))
		->execute();

	$tids = $query->fetchCol('tid');

	// If there are any transactions on this account, load a list of alternative MAs these transactions can be transferred to
	if (count($tids)) {
		$query = new EntityFieldQuery;
		$query->entityCondition('entity_type', 'fratler_master')
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

		$alt_accounts = fratler_master_load_multiple(array_keys($results['fratler_master']));

		foreach ($alt_accounts as $alt_account)
			$options[$alt_account->mid] = $alt_account->name;

		debug($alt_accounts, 'alts');
	}

	$form['cont'] = array(
		'#type'		=> 'fieldset',
		'#title'	=> t('Confirm deletion'),
		'#description'	=> t('Please confirm you want to delete this Master Account. <b>This action cannot be undone.</b>'),
		'#collapsible'	=> false,
	);

	if (count($tids)) {
		$form['cont']['alt_acc'] = array(
			'#type'		=> 'select',
			'#title'	=> t('Alternative Master Account'),
			'#description'	=> format_plural(count($tids), 'There is currently <b>one</b> transaction on this Master Account. Please select an alternate Master Account which will hold the transaction.',
									'There are currently <b>@count</b> transactions on this Master Account. Please select an alternate Master Account which will hold the transactions.'),
			'#options'	=> $options,
			'#required'	=> true,
		);
		$form['tids'] = array(
			'#type'		=> 'value',
			'#value'	=> $tids,
		);
	}

	$form['cont']['confirm'] = array(
		'#type'		=> 'checkbox',
		'#title'	=> t('I know what I am doing'),
		'#required'	=> true,
	);
	$form['account'] = array(
		'#type'		=> 'value',
		'#value'	=> $account,
	);
	$form['cont']['actions'] = array(
		'#type'		=> 'actions',
	);
	$form['cont']['actions']['submit'] = array(
		'#type'		=> 'submit',
		'#value'	=> t('Confirm'),
	);
	$form['cont']['actions']['cancel'] = array(
		'#markup'	=> l(t('Cancel'), 'admin/config/fratler/master'),
	);
	return $form;
}

function fratler_master_admin_delete_submit($form, &$form_state) {
	debug(__FUNCTION__);
	$account 	= $form_state['values']['account'];
	// Ok, before we can do shit, we have to move shit.
	if (isset($form_state['values']['tids'])) {
		$tids 		= $form_state['values']['tids'];
		$alt_account 	= $form_state['values']['alt_acc'];

		foreach ($tids as $tid) {
			fratler_money_reassign($tid, $account->mid, $alt_account);
		}
	}	

	fratler_master_delete($account);
	drupal_set_message(t('Master Account deleted.'), 'status');
	$form_state['redirect'] = 'admin/config/fratler/master';
}

