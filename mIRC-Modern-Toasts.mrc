on *:text:*:#:{
  if ($highlight && $highlight($1-) && ($1- !isnum) && !$appactive) {
    ; this is just personal preference, feel free to change to your needs
    window -x $chan
    noop $mmt-tip(Highlight: $chan,< $+ $nick $+ > $1-)
  }
}

alias mmt-tip {
  $dll(mIRC-Modern-Toasts.dll, SetLine1, $$1)
  $dll(mIRC-Modern-Toasts.dll, SetLine2, $$2)
  $dllcall(mIRC-Modern-Toasts.dll, noop, ShowToast, $null)
}

alias mmt-toast-activate-callback {
  showmirc -s
}

alias mmt-toast-fail-callback {
}

alias mmt-toast-dismiss-callback {
}

