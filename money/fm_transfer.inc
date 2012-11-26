<?php

// Set the locale for formatting numbers
setlocale(LC_MONETARY, variable_get('fm_locale', 'C'));
$locale = localeconv();

/**
 * Page-Callback for transactions
 * @see fratler_money_menu()
 */
function fm_transfer($form, &$form_state) {
	global $user;
	if (!function_exists('_fm_strip'))
		module_load_include('inc', 'fratler_money', 'fm_tools');
	$locale = localeconv();
	$adjust = false;
	if (in_array('adjust', $form_state['build_info']['args']))
		$adjust = true;

	// Display default transaction form
	if(!isset($form_state['storage']['confirm'])) {	
		
		$form['general'] = array(
			'#type'		=> 'fieldset',
			'#title'	=> t('Transaction data'),
		);

		// If the user is allowed to transfer money from other accounts, display a "From" field
		$form['general']['sid'] = array(
			'#type'		=> 'textfield',
			'#title'	=> t('Sender'),
			'#size'		=> 20,
			'#maxlength'	=> 60,
			'#required'	=> true,
			'#default_value' => $user->name,
			'#autocomplete_path' => 'fratler/callback/money/transfer',
		);
		// If the user is not allowed to make 3rd party transactions or
		// it's an adjustment, hide (and disable) the field
		if (!user_access('fm transfer other-accounts') || $adjust) {
			$form['general']['sid']['#type'] 	= 'hidden';
			if ($adjust)
				$form['general']['sid']['#disabled']	= true;
		}

		$form['general']['uid'] = array(
			'#type' 	=> 'textfield',
			'#title' 	=> $adjust ? t('Account') : t('Beneficiary'),
			'#size' 	=> 20,
			'#maxlength' 	=> 60,
			'#required'	=> true,
			'#autocomplete_path' => 'fratler/callback/money/transfer'
		);
		$form['general']['amount'] = array(
			'#title' 	=> t('Amount'),
			'#type'		=> 'textfield',
			'#field_prefix'	=> $locale['p_cs_precedes'] ? _fm_cs() : '',
			'#field_suffix'	=> !$locale['p_cs_precedes'] ? _fm_cs() : '',
			'#size'		=> 5,
			'#maxlength'	=> 7,
			'#default_value' => money_format("%!n", 0),
			'#required'	=> true,
		);
		$form['general']['text'] = array(
			'#title' 	=> t('Message'),
			'#type'		=> 'textfield',
			'#maxlength'	=> 255,
		);
		$form['general']['type'] = array(
			'#type'		=> 'hidden',
			'#default_value' => variable_get('fm_vocab_transfer_term'),
		);
		$form['general']['submit'] = array(
			'#value'	=> t('Submit'),
			'#type'		=> 'submit',
		);

		return $form;
	}
	// Confirm-Page
	else {
		$form['general']['sid'] = array(
			'#type'		=> 'hidden',
			'#title'	=> t('Sender'),
			'#size'		=> 20,
			'#maxlength'	=> 60,
			'#required'	=> true,
		);
		if ($adjust)
			$form['general']['sid']['#disabled'] = true;

		$form['general']['uid'] = array(
			'#type' 	=> 'hidden',
			'#title' 	=> t('Beneficiary'),
			'#size' 	=> 20,
			'#maxlength' 	=> 60,
			'#required'	=> true,
		);
		$form['general']['amount'] = array(
			'#title' 	=> t('Amount'),
			'#type'		=> 'hidden',
			'#size'		=> 5,
			'#maxlength'	=> 7,
			'#required'	=> true,
		);
		$form['general']['text'] = array(
			'#title' 	=> t('Message'),
			'#type'		=> 'hidden',
			'#maxlength'	=> 255,
		);
		$form['general']['type'] = array(
			'#title'	=> t('Transaction type'),
			'#type'		=> 'hidden',
		);

		drupal_add_css(drupal_get_path('module', 'fratler_money') . '/styles/fratler_money.css');

		$header = array(t('Sender'), $adjust ? t('Account') : t('Recipient'), t('Type'), t('Amount'), array('data' => t('Message'), 'class' => array('fm_message')), array('data' => t('New balance'), 'class' => array('fm_header')));
		$sid = ($form_state['values']['sid'] == $user->name) ? $user : (empty($form_state['values']['sid']) ? false : user_load_by_name($form_state['values']['sid']));
		$uid = ($form_state['values']['uid'] == $user->name) ? $user : user_load_by_name($form_state['values']['uid']);
		$row = array(array(
			format_username($sid),
			format_username($uid),
			taxonomy_term_load($form_state['values']['type'])->name,
			array('data' => money_format("%n", $form_state['values']['amount']), 'class' => array('fm_amount')),
			_fm_strip_html($form_state['values']['text']),
			array('data' => money_format("%n", ($adjust ? _fm_balance($uid->uid) + _fm_parse_float($form_state['values']['amount']) : _fm_balance($sid->uid) - _fm_parse_float($form_state['values']['amount']))), 
				'class' => array('fm_amount', ($adjust ? _fm_balance($uid->uid) + _fm_parse_float($form_state['values']['amount']) : _fm_balance($sid->uid) - _fm_parse_float($form_state['values']['amount'])) < 0 ? 'fm_debit' : 'fm_credit' )),
			),
		array(
			array('data' => ' --- ' . t('Please double-check your data') . ' --- ', 'colspan' => 5, 'class' => array('fm_header', 'fm_center')),
		));

		// If we're adjusting, remove the first entries of the header and the row
		if ($adjust) {
			array_shift($header);
			array_shift($row[0]);
		}


		$html = theme('table',  array(
			'header'        => $header,
			'empty'         => t('No transactions available'),
			'rows'          => $row,
			'sticky'        => true,
			'attributes'	=> array('class' => array('fm_singlet')),
		));


		return confirm_form($form, 
			t('Are you sure you want to proceed?'),
			isset($_GET['destination']) ? $_GET['destination'] : 'fratler/money/transfer/' . ($adjust ? 'adjust' : 'normal'), $html);
	}

}

/**
 * Check the transfer-data
 * Implements hook_validate()
 */
function fm_transfer_validate($form, &$form_state) {
	global $user;
	if (!function_exists('_fm_strip'))
		module_load_include('inc', 'fratler_money', 'fm_tools');
	$adjust = false;
	if (in_array('adjust', $form_state['build_info']['args']))
		$adjust = true;


	// Check the entered sender
	if (!$adjust) {
		$sender = ($user->name == $form_state['values']['sid']) ? $user : user_load_by_name($form_state['values']['sid']);

		 if (!$sender)
			form_set_error('sid', t('Sender Invalid'));
		 elseif ($form_state['values']['sid'] != $user->name) {	// check permissions	
			if (user_access('fm transfer other-accounts'))
				drupal_set_message(t('You are conducting a third-party transfer.'), 'warning', false);
			else	// User is not allowed to transfer away.
				form_set_error('sid', t('You are not allowed to make third-party transactions.'));
		 } 
		if (!_fm_check_participant($sender))       // Sender may not participate in transfers
			if (user_access('fm transfer non-participants'))        // ...but the conducting user is excluded from that rule
				drupal_set_message(t('Sender is not allowed to issue transactions.'), 'warning', false);
			else                                                    // issue an error
				form_set_error('uid', t('Sender may not participate in transfers.'));
	}

	// Check the entered recipient / the account that's going to be modified
	$recipient = user_load_by_name(check_plain($form_state['values']['uid']));

	if (!$recipient) 	// Recipient does not exists
		form_set_error('uid', t('Recipient invalid.'));
	elseif (!$adjust && $recipient->uid == $sender->uid)
		form_set_error('uid', t('Sender and recipient may not be the same, genius!'));
	elseif (!_fm_check_participant($recipient))	// Recipient may not participate in transfers
		if (user_access('fm transfer non-participants'))	// ...but the conducting user is excluded from that rule
			drupal_set_message(t('Recipient may not participate in transfers.'), 'warning', false);
		else							// issue an error
			form_set_error('uid', t('Recipient may not participate in transfers.'));
	
	// Perform checks on the entered amount:
	if (!is_numeric(_fm_strip($form_state['values']['amount'])) || (!$adjust && _fm_parse_float($form_state['values']['amount']) < 0) || _fm_parse_float($form_state['values']['amount']) == 0) {		// skip all further checks if the amount is invalid
		form_set_error('amount', t('Amount invalid.'));
		return;
	}
	if (!$adjust && _fm_parse_float($form_state['values']['amount']) > variable_get('fm_trans_max', 0) && variable_get('fm_trans_max', 0) != 0)
		if (user_access('fm transfer exceed limit'))
			drupal_set_message(t('Amount exceeds maximum transaction volume.'), 'warning', false);
		else
			form_set_error('amount', t('Amount exceeds maximum transaction volume.'));

	// Check if the user's balance is < 0 or exceeds the overdraw-limit  (not relevant for adjustments)
	if (!$adjust && _fm_balance($sender->uid) - _fm_parse_float($form_state['values']['amount']) < 0)
		if (_fm_balance($sender->uid) - _fm_parse_float($form_state['values']['amount']) < variable_get('fm_overdraw_limit', 0)) {	// we're dropping below the overdraw-limit
			if (user_access('fm overdraw'))			// ...and the user may overdraw, issue a warning
				drupal_set_message(t("Transaction value exceeds @user account-limit.", array('@user' => ($user->uid != $sender->uid) ? format_username($sender) . t("'s",array(),array('context' => 'Possesive suffix')) : t('your'))), 'warning', false);
			else						// the user may not overdraw
				form_set_error('amount', t("Transaction value exceeds @user account-limit.", array('@user' => ($user->uid != $sender->uid) . t("'s",array(),array('context' => 'Possesive suffix')) ? format_username($sender) : t('your'))));
		}
	else	// nevertheless, we're below 0 so let's issue a curtesy-warning, unless we're adjusting
		if(!$adjust)
			drupal_set_message(t('This transaction will overdraw @user account.', array('@user' => ($user->uid != $sender->uid) ? format_username($sender) . t("'s",array(),array('context' => 'Possesive suff ix')) : t('your'))), 'warning', false);

	form_set_value(array('#parents' => array('text')), _fm_strip_html($form_state['values']['text']), $form_state);
	form_set_value(array('#parents' => array('amount')), _fm_parse_float($form_state['values']['amount']), $form_state);

	// Adjust the transaction-type, in case we're adjusting...
	if ($adjust)
		form_set_value(array('#parents' => array('type')), _fm_parse_float($form_state['values']['amount']) > 0 ? variable_get('fm_vocab_adjust_deposit') : variable_get('fm_vocab_adjust_withdraw'), $form_state);

}

class fm_transaction {
	private $sid;		// User-Object of the sender
	private $rid;		// User-Object of the beneficiary
	private $iid;		// User-Object of the initiating user
	private $type;		// Transaction-Type (Taxonomy Term-ID)
	private $amount;	// Schnitzel 'n stuff...
	private $text;		// The text that goes with the transaction
	private $timestamp;	// The timestamp will be generated when the constructor is called
	private $md5sum;	// The fingerprint that's generated per transaction
	private $signature_in;	// The OpenSSL-signature that's generated for the incoming transaction 
	private $signature_out;	// The OpenSSL-signature that's generated for the outgoing transaction

	function __construct($sid, $rid, $iid, $amount, $text, $type) {
		// fetch my toolbox
		if (!function_exists('_fm_strip_html'))
			module_load_include('inc', 'fratler_money', 'fm_tools');

		// Since somewhere down the road, I'll get lazy or forget what kind of user i need to pass, let's include some extras:
		// Check for transaction-type first, if we're adjusting; if so: set sid to 0
		if ($type == variable_get('fm_vocab_adjust_withdraw') || $type == variable_get('fm_vocab_adjust_deposit'))
			$this->sid = (object) array('uid' => 0, 'name' => '');
		elseif (is_numeric($sid))
			$this->sid = user_load($sid);
		elseif (is_string($sid))
			$this->sid = user_load_by_name($sid);
		elseif (is_object($sid))
			$this->sid = $sid;
		
		if (is_numeric($rid))
			$this->rid = user_load($rid);
		elseif (is_string($rid))
			$this->rid = user_load_by_name($rid);
		elseif (is_object($rid))
			$this->rid = $rid;

		if (is_numeric($iid) && $iid != 0) 
			$this->iid = user_load($iid);
		elseif (is_string($iid)) 
			$this->iid = user_load_by_name($iid);
		elseif (is_object($iid)) 
			$this->iid = $iid;
		else {
			$this->iid = new stdClass();
			$this->iid->uid = 0;
		}

		$this->amount = number_format($amount, 2, '.','');

		$this->text = _fm_strip_html($text);

		$this->timestamp = time();

		$this->type = $type;
	}

	function commit() {
		// Step 1: Calculate the Fingerprint
		$this->md5sum 		= _fm_hash($this->sid->uid, $this->rid->uid, $this->iid->uid, $this->type, $this->amount, $this->text, $this->timestamp);

		// Step 2: Get current balances
		$balance_rid = _fm_balance($this->rid->uid);
			// Skip if adjusting
		if (!$this->sid->uid == 0) $balance_sid = _fm_balance($this->sid->uid);

		// Step 3: Sign transactions
		$this->signature_in 	= _fm_sign($this->rid->uid, $this->sid->uid, $this->iid->uid, $this->type, $this->amount, $this->text, $balance_rid+floatval($this->amount), $this->timestamp);
			// Skip outgoing if adjusting
		if (!$this->sid->uid == 0) $this->signature_out	= _fm_sign($this->sid->uid, $this->rid->uid, $this->iid->uid, $this->type, - $this->amount, $this->text, $balance_sid-floatval($this->amount),  $this->timestamp);
		error_log("Checking in type: " . $this->type);

		// Step 4: Create incoming transaction
		$tid_in = db_insert('fratler_money')->fields(array(
			'rid' 		=> $this->rid->uid,
			'sid' 		=> $this->sid->uid,
			'iid' 		=> $this->iid->uid,
			'type'		=> $this->type,
			'text'	 	=> $this->text,
		        'amount'	=> floatval($this->amount),
			'timestamp' 	=> $this->timestamp,
			'fingerprint'	=> $this->md5sum,
			'signature'	=> $this->signature_in,
			'balance'	=> $balance_rid+floatval($this->amount),
		))->execute();

		// Step 5: Create outgoing transaction (if not adjusting...)
		if (!$this->sid->uid == 0)
			$tid_out = db_insert('fratler_money')->fields(array(
				'rid' 		=> $this->sid->uid,
				'sid' 		=> $this->rid->uid,
				'iid' 		=> $this->iid->uid,
				'type'		=> $this->type,
				'text'	 	=> $this->text,
				'amount'	=> -floatval($this->amount),
				'timestamp' 	=> $this->timestamp,
				'fingerprint'	=> $this->md5sum,
				'signature'	=> $this->signature_out,
				'balance'	=> $balance_sid-floatval($this->amount),
			))->execute();	

		// Step 6: Update the balances
		db_merge('fratler_money_balances')->key(array('uid' => $this->rid->uid))->fields(array(
			'balance' 	=> $balance_rid+floatval($this->amount),
			'timestamp'	=> $this->timestamp,
			'last_trans'	=> $tid_in,
		))->execute();
			// Guess what...
		if (!$this->sid->uid == 0)
			db_merge('fratler_money_balances')->key(array('uid' => $this->sid->uid))->fields(array(
				'balance' 	=> $balance_sid-floatval($this->amount),
				'timestamp'	=> $this->timestamp,
				'last_trans'	=> $tid_out,
			))->execute();

		drupal_set_message(t('Transaction successful'), 'status', false);
		return $this->md5sum;
	}

}

/**
 * Implements hook_submit()
 */
function fm_transfer_submit($form, &$form_state) {
	if(!isset($form_state['storage']['confirm'])) {		// Issue a rebuild-order to display the confirm-Page
		$form_state['storage']['confirm'] = true;
		$form_state['rebuild'] = true;
		return;
	}

	global $user;

	$adjust = false;
	if (in_array('adjust', $form_state['build_info']['args']))
		$adjust = true;


	// Alright, let's ring'er up.
	$transaction 	= new fm_transaction($form_state['values']['sid'], $form_state['values']['uid'], ($user->name == $form_state['values']['sid']) ? 0 : $user, $form_state['values']['amount'], $form_state['values']['text'], $form_state['values']['type']);
	$result		= $transaction->commit();

	// Redirect to a confirmation page 
	$form_state['redirect'] = '/fratler/money/display/' . $result;
}
