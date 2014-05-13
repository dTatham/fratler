<?php
/* vim: set filetype=php expandtab tabstop=2 shiftwidth=2 autoindent smartindent: */


function _fac_nuke() {
  global $user;
  $build = '';

  $status = _fac_permission_calculate($user);

  $status         = 0;
  $all_permissions        = _fac_permission_render(FAcPerm::ALL_FAC | FAcPerm::ALL_TRANS);
  $user_roles     = user_role_permissions($user->roles);

  $flattened      = array();
  array_walk_recursive($user_roles, function($a,$b) use (&$flattened) { $flattened[] = $b; });
  debug($flattened, 'flat');
  debug($all_permissions, 'all');

  foreach ($flattened as $checkme) {
    if (array_key_exists($checkme, $all_permissions))
    $status |= $all_permissions[$checkme]['key'];
  }

  if ($status & FAcPerm::RECEIVE || $status & FAcPerm::TRANSFER || $status & FAcPerm::ADJUST || $status & FAcPerm::DISPLAY)
    $status |= FAcPerm::PARTICIPATE;

  debug($status);

  return array('#markup' => "<pre>$build</pre>");
}
