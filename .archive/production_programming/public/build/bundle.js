
(function(l, r) {
if (l.getElementById('livereloadscript'))
  return;
r = l.createElement('script');
r.async = 1;
r.src = '//' + (window.location.host || 'localhost').split(':')[0] +
        ':35729/livereload.js?snipver=1';
r.id = 'livereloadscript';
l.getElementsByTagName('head')[0].appendChild(r)
})(window.document);
var app = (function() {
  'use strict';

  function noop() {}
  function add_location(element, file, line, column, char) {
    element.__svelte_meta = {loc : {file, line, column, char}};
  }
  function run(fn) { return fn(); }
  function blank_object() { return Object.create(null); }
  function run_all(fns) { fns.forEach(run); }
  function is_function(thing) { return typeof thing === 'function'; }
  function safe_not_equal(a, b) {
    return a != a ? b == b
                  : a !== b || ((a && typeof a === 'object') ||
                                typeof a === 'function');
  }
  function is_empty(obj) { return Object.keys(obj).length === 0; }

  function append(target, node) { target.appendChild(node); }
  function insert(target, node, anchor) {
    target.insertBefore(node, anchor || null);
  }
  function detach(node) { node.parentNode.removeChild(node); }
  function destroy_each(iterations, detaching) {
    for (let i = 0; i < iterations.length; i += 1) {
      if (iterations[i])
        iterations[i].d(detaching);
    }
  }
  function element(name) { return document.createElement(name); }
  function text(data) { return document.createTextNode(data); }
  function space() { return text(' '); }
  function empty() { return text(''); }
  function listen(node, event, handler, options) {
    node.addEventListener(event, handler, options);
    return () => node.removeEventListener(event, handler, options);
  }
  function attr(node, attribute, value) {
    if (value == null)
      node.removeAttribute(attribute);
    else if (node.getAttribute(attribute) !== value)
      node.setAttribute(attribute, value);
  }
  function children(element) { return Array.from(element.childNodes); }
  function set_input_value(input, value) {
    input.value = value == null ? '' : value;
  }
  function custom_event(type, detail) {
    const e = document.createEvent('CustomEvent');
    e.initCustomEvent(type, false, false, detail);
    return e;
  }

  let current_component;
  function set_current_component(component) { current_component = component; }
  function get_current_component() {
    if (!current_component)
      throw new Error('Function called outside component initialization');
    return current_component;
  }
  function onMount(fn) { get_current_component().$$.on_mount.push(fn); }

  const dirty_components = [];
  const binding_callbacks = [];
  const render_callbacks = [];
  const flush_callbacks = [];
  const resolved_promise = Promise.resolve();
  let update_scheduled = false;
  function schedule_update() {
    if (!update_scheduled) {
      update_scheduled = true;
      resolved_promise.then(flush);
    }
  }
  function add_render_callback(fn) { render_callbacks.push(fn); }
  let flushing = false;
  const seen_callbacks = new Set();
  function flush() {
    if (flushing)
      return;
    flushing = true;
    do {
      // first, call beforeUpdate functions
      // and update components
      for (let i = 0; i < dirty_components.length; i += 1) {
        const component = dirty_components[i];
        set_current_component(component);
        update(component.$$);
      }
      set_current_component(null);
      dirty_components.length = 0;
      while (binding_callbacks.length)
        binding_callbacks.pop()();
      // then, once components are updated, call
      // afterUpdate functions. This may cause
      // subsequent updates...
      for (let i = 0; i < render_callbacks.length; i += 1) {
        const callback = render_callbacks[i];
        if (!seen_callbacks.has(callback)) {
          // ...so guard against infinite loops
          seen_callbacks.add(callback);
          callback();
        }
      }
      render_callbacks.length = 0;
    } while (dirty_components.length);
    while (flush_callbacks.length) {
      flush_callbacks.pop()();
    }
    update_scheduled = false;
    flushing = false;
    seen_callbacks.clear();
  }
  function update($$) {
    if ($$.fragment !== null) {
      $$.update();
      run_all($$.before_update);
      const dirty = $$.dirty;
      $$.dirty = [ -1 ];
      $$.fragment && $$.fragment.p($$.ctx, dirty);
      $$.after_update.forEach(add_render_callback);
    }
  }
  const outroing = new Set();
  function transition_in(block, local) {
    if (block && block.i) {
      outroing.delete(block);
      block.i(local);
    }
  }

  const globals = (typeof window !== 'undefined'       ? window
                   : typeof globalThis !== 'undefined' ? globalThis
                                                       : global);
  function mount_component(component, target, anchor, customElement) {
    const {fragment, on_mount, on_destroy, after_update} = component.$$;
    fragment && fragment.m(target, anchor);
    if (!customElement) {
      // onMount happens before the initial afterUpdate
      add_render_callback(() => {
        const new_on_destroy = on_mount.map(run).filter(is_function);
        if (on_destroy) {
          on_destroy.push(...new_on_destroy);
        } else {
          // Edge case - component was destroyed immediately,
          // most likely as a result of a binding initialising
          run_all(new_on_destroy);
        }
        component.$$.on_mount = [];
      });
    }
    after_update.forEach(add_render_callback);
  }
  function destroy_component(component, detaching) {
    const $$ = component.$$;
    if ($$.fragment !== null) {
      run_all($$.on_destroy);
      $$.fragment && $$.fragment.d(detaching);
      // TODO null out other refs, including component.$$ (but need to
      // preserve final state?)
      $$.on_destroy = $$.fragment = null;
      $$.ctx = [];
    }
  }
  function make_dirty(component, i) {
    if (component.$$.dirty[0] === -1) {
      dirty_components.push(component);
      schedule_update();
      component.$$.dirty.fill(0);
    }
    component.$$.dirty[(i / 31) | 0] |= (1 << (i % 31));
  }
  function init(component, options, instance, create_fragment, not_equal, props,
                dirty = [ -1 ]) {
    const parent_component = current_component;
    set_current_component(component);
    const $$ = component.$$ = {
      fragment : null,
      ctx : null,
      // state
      props,
      update : noop,
      not_equal,
      bound : blank_object(),
      // lifecycle
      on_mount : [],
      on_destroy : [],
      on_disconnect : [],
      before_update : [],
      after_update : [],
      context : new Map(parent_component ? parent_component.$$.context : []),
      // everything else
      callbacks : blank_object(),
      dirty,
      skip_bound : false
    };
    let ready = false;
    $$.ctx = instance ? instance(component, options.props || {},
                                 (i, ret, ...rest) => {
                                   const value = rest.length ? rest[0] : ret;
                                   if ($$.ctx && not_equal($$.ctx[i],
                                                           $$.ctx[i] = value)) {
                                     if (!$$.skip_bound && $$.bound[i])
                                       $$.bound[i](value);
                                     if (ready)
                                       make_dirty(component, i);
                                   }
                                   return ret;
                                 })
                      : [];
    $$.update();
    ready = true;
    run_all($$.before_update);
    // `false` as a special case of no DOM component
    $$.fragment = create_fragment ? create_fragment($$.ctx) : false;
    if (options.target) {
      if (options.hydrate) {
        const nodes = children(options.target);
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        $$.fragment && $$.fragment.l(nodes);
        nodes.forEach(detach);
      } else {
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        $$.fragment && $$.fragment.c();
      }
      if (options.intro)
        transition_in(component.$$.fragment);
      mount_component(component, options.target, options.anchor,
                      options.customElement);
      flush();
    }
    set_current_component(parent_component);
  }
  /**
   * Base class for Svelte components. Used when dev=false.
   */
  class SvelteComponent {
    $destroy() {
      destroy_component(this, 1);
      this.$destroy = noop;
    }
    $on(type, callback) {
      const callbacks =
          (this.$$.callbacks[type] || (this.$$.callbacks[type] = []));
      callbacks.push(callback);
      return () => {
        const index = callbacks.indexOf(callback);
        if (index !== -1)
          callbacks.splice(index, 1);
      };
    }
    $set($$props) {
      if (this.$$set && !is_empty($$props)) {
        this.$$.skip_bound = true;
        this.$$set($$props);
        this.$$.skip_bound = false;
      }
    }
  }

  function dispatch_dev(type, detail) {
    document.dispatchEvent(
        custom_event(type, Object.assign({version : '3.35.0'}, detail)));
  }
  function append_dev(target, node) {
    dispatch_dev('SvelteDOMInsert', {target, node});
    append(target, node);
  }
  function insert_dev(target, node, anchor) {
    dispatch_dev('SvelteDOMInsert', {target, node, anchor});
    insert(target, node, anchor);
  }
  function detach_dev(node) {
    dispatch_dev('SvelteDOMRemove', {node});
    detach(node);
  }
  function listen_dev(node, event, handler, options, has_prevent_default,
                      has_stop_propagation) {
    const modifiers = options === true ? [ 'capture' ]
                      : options        ? Array.from(Object.keys(options))
                                       : [];
    if (has_prevent_default)
      modifiers.push('preventDefault');
    if (has_stop_propagation)
      modifiers.push('stopPropagation');
    dispatch_dev('SvelteDOMAddEventListener',
                 {node, event, handler, modifiers});
    const dispose = listen(node, event, handler, options);
    return () => {
      dispatch_dev('SvelteDOMRemoveEventListener',
                   {node, event, handler, modifiers});
      dispose();
    };
  }
  function attr_dev(node, attribute, value) {
    attr(node, attribute, value);
    if (value == null)
      dispatch_dev('SvelteDOMRemoveAttribute', {node, attribute});
    else
      dispatch_dev('SvelteDOMSetAttribute', {node, attribute, value});
  }
  function set_data_dev(text, data) {
    data = '' + data;
    if (text.wholeText === data)
      return;
    dispatch_dev('SvelteDOMSetData', {node : text, data});
    text.data = data;
  }
  function validate_each_argument(arg) {
    if (typeof arg !== 'string' &&
        !(arg && typeof arg === 'object' && 'length' in arg)) {
      let msg = '{#each} only iterates over array-like objects.';
      if (typeof Symbol === 'function' && arg && Symbol.iterator in arg) {
        msg += ' You can use a spread to convert this iterable into an array.';
      }
      throw new Error(msg);
    }
  }
  function validate_slots(name, slot, keys) {
    for (const slot_key of Object.keys(slot)) {
      if (!~keys.indexOf(slot_key)) {
        console.warn(`<${name}> received an unexpected slot "${slot_key}".`);
      }
    }
  }
  /**
   * Base class for Svelte components with some minor dev-enhancements. Used
   * when dev=true.
   */
  class SvelteComponentDev extends SvelteComponent {
    constructor(options) {
      if (!options || (!options.target && !options.$$inline)) {
        throw new Error("'target' is a required option");
      }
      super();
    }
    $destroy() {
      super.$destroy();
      this.$destroy = () => {
        console.warn('Component was already destroyed'); // eslint-disable-line
                                                         // no-console
      };
    }
    $capture_state() {}
    $inject_state() {}
  }

  /* svelte/App.svelte generated by Svelte v3.35.0 */

  const {console : console_1} = globals;
  const file = "svelte/App.svelte";

  function get_each_context(ctx, list, i) {
    const child_ctx = ctx.slice();
    child_ctx[30] = list[i];
    return child_ctx;
  }

  function get_each_context_1(ctx, list, i) {
    const child_ctx = ctx.slice();
    child_ctx[33] = list[i];
    return child_ctx;
  }

  function get_each_context_2(ctx, list, i) {
    const child_ctx = ctx.slice();
    child_ctx[33] = list[i];
    return child_ctx;
  }

  function get_each_context_3(ctx, list, i) {
    const child_ctx = ctx.slice();
    child_ctx[33] = list[i];
    return child_ctx;
  }

  function get_each_context_4(ctx, list, i) {
    const child_ctx = ctx.slice();
    child_ctx[33] = list[i];
    return child_ctx;
  }

  // (391:3) {#each mcu_pins[0] as pin}
  function create_each_block_4(ctx) {
    let div;
    let t_value = /*pin*/ ctx[33].function + "";
    let t;
    let div_class_value;

    const block = {
      c : function create() {
        div = element("div");
        t = text(t_value);
        attr_dev(div, "class",
                 div_class_value =
                     "pin " + /*pin*/ ctx[33].class + " svelte-16idiwm");
        add_location(div, file, 391, 4, 10246);
      },
      m : function mount(target, anchor) {
        insert_dev(target, div, anchor);
        append_dev(div, t);
      },
      p : function update(ctx, dirty) {
        if (dirty[0] & /*mcu_pins*/ 256 &&
            t_value !== (t_value = /*pin*/ ctx[33].function + ""))
          set_data_dev(t, t_value);

        if (dirty[0] & /*mcu_pins*/ 256 &&
            div_class_value !==
                (div_class_value =
                     "pin " + /*pin*/ ctx[33].class + " svelte-16idiwm")) {
          attr_dev(div, "class", div_class_value);
        }
      },
      d : function destroy(detaching) {
        if (detaching)
          detach_dev(div);
      }
    };

    dispatch_dev("SvelteRegisterBlock", {
      block,
      id : create_each_block_4.name,
      type : "each",
      source : "(391:3) {#each mcu_pins[0] as pin}",
      ctx
    });

    return block;
  }

  // (396:2) {#each mcu_pins[1] as pin}
  function create_each_block_3(ctx) {
    let div;
    let t_value = /*pin*/ ctx[33].function + "";
    let t;
    let div_class_value;

    const block = {
      c : function create() {
        div = element("div");
        t = text(t_value);
        attr_dev(div, "class",
                 div_class_value =
                     "pin " + /*pin*/ ctx[33].class + " svelte-16idiwm");
        add_location(div, file, 396, 3, 10375);
      },
      m : function mount(target, anchor) {
        insert_dev(target, div, anchor);
        append_dev(div, t);
      },
      p : function update(ctx, dirty) {
        if (dirty[0] & /*mcu_pins*/ 256 &&
            t_value !== (t_value = /*pin*/ ctx[33].function + ""))
          set_data_dev(t, t_value);

        if (dirty[0] & /*mcu_pins*/ 256 &&
            div_class_value !==
                (div_class_value =
                     "pin " + /*pin*/ ctx[33].class + " svelte-16idiwm")) {
          attr_dev(div, "class", div_class_value);
        }
      },
      d : function destroy(detaching) {
        if (detaching)
          detach_dev(div);
      }
    };

    dispatch_dev("SvelteRegisterBlock", {
      block,
      id : create_each_block_3.name,
      type : "each",
      source : "(396:2) {#each mcu_pins[1] as pin}",
      ctx
    });

    return block;
  }

  // (403:3) {#each mcu_pins[2] as pin}
  function create_each_block_2(ctx) {
    let div;
    let t_value = /*pin*/ ctx[33].function + "";
    let t;
    let div_class_value;

    const block = {
      c : function create() {
        div = element("div");
        t = text(t_value);
        attr_dev(div, "class",
                 div_class_value =
                     "pin " + /*pin*/ ctx[33].class + " svelte-16idiwm");
        add_location(div, file, 403, 4, 10509);
      },
      m : function mount(target, anchor) {
        insert_dev(target, div, anchor);
        append_dev(div, t);
      },
      p : function update(ctx, dirty) {
        if (dirty[0] & /*mcu_pins*/ 256 &&
            t_value !== (t_value = /*pin*/ ctx[33].function + ""))
          set_data_dev(t, t_value);

        if (dirty[0] & /*mcu_pins*/ 256 &&
            div_class_value !==
                (div_class_value =
                     "pin " + /*pin*/ ctx[33].class + " svelte-16idiwm")) {
          attr_dev(div, "class", div_class_value);
        }
      },
      d : function destroy(detaching) {
        if (detaching)
          detach_dev(div);
      }
    };

    dispatch_dev("SvelteRegisterBlock", {
      block,
      id : create_each_block_2.name,
      type : "each",
      source : "(403:3) {#each mcu_pins[2] as pin}",
      ctx
    });

    return block;
  }

  // (408:3) {#each mcu_pins[3] as pin}
  function create_each_block_1(ctx) {
    let div;
    let t_value = /*pin*/ ctx[33].function + "";
    let t;
    let div_class_value;

    const block = {
      c : function create() {
        div = element("div");
        t = text(t_value);
        attr_dev(div, "class",
                 div_class_value =
                     "pin " + /*pin*/ ctx[33].class + " svelte-16idiwm");
        add_location(div, file, 408, 4, 10641);
      },
      m : function mount(target, anchor) {
        insert_dev(target, div, anchor);
        append_dev(div, t);
      },
      p : function update(ctx, dirty) {
        if (dirty[0] & /*mcu_pins*/ 256 &&
            t_value !== (t_value = /*pin*/ ctx[33].function + ""))
          set_data_dev(t, t_value);

        if (dirty[0] & /*mcu_pins*/ 256 &&
            div_class_value !==
                (div_class_value =
                     "pin " + /*pin*/ ctx[33].class + " svelte-16idiwm")) {
          attr_dev(div, "class", div_class_value);
        }
      },
      d : function destroy(detaching) {
        if (detaching)
          detach_dev(div);
      }
    };

    dispatch_dev("SvelteRegisterBlock", {
      block,
      id : create_each_block_1.name,
      type : "each",
      source : "(408:3) {#each mcu_pins[3] as pin}",
      ctx
    });

    return block;
  }

  // (421:4) {#if entry.context == "uart" && uart_console_enabled}
  function create_if_block_3(ctx) {
    let div;
    let t_value = /*entry*/ ctx[30].data + "";
    let t;
    let div_class_value;

    const block = {
      c : function create() {
        div = element("div");
        t = text(t_value);
        attr_dev(div, "class",
                 div_class_value = "consoleline " + /*entry*/ ctx[30].context +
                                   " svelte-16idiwm");
        add_location(div, file, 421, 5, 10882);
      },
      m : function mount(target, anchor) {
        insert_dev(target, div, anchor);
        append_dev(div, t);
      },
      p : function update(ctx, dirty) {
        if (dirty[0] & /*ui_console*/ 64 &&
            t_value !== (t_value = /*entry*/ ctx[30].data + ""))
          set_data_dev(t, t_value);

        if (dirty[0] & /*ui_console*/ 64 &&
            div_class_value !==
                (div_class_value = "consoleline " + /*entry*/ ctx[30].context +
                                   " svelte-16idiwm")) {
          attr_dev(div, "class", div_class_value);
        }
      },
      d : function destroy(detaching) {
        if (detaching)
          detach_dev(div);
      }
    };

    dispatch_dev("SvelteRegisterBlock", {
      block,
      id : create_if_block_3.name,
      type : "if",
      source :
          "(421:4) {#if entry.context == \\\"uart\\\" && uart_console_enabled}",
      ctx
    });

    return block;
  }

  // (424:4) {#if entry.context == "openocd" && openocd_console_enabled}
  function create_if_block_2(ctx) {
    let div;
    let t_value = /*entry*/ ctx[30].data + "";
    let t;
    let div_class_value;

    const block = {
      c : function create() {
        div = element("div");
        t = text(t_value);
        attr_dev(div, "class",
                 div_class_value = "consoleline " + /*entry*/ ctx[30].context +
                                   " svelte-16idiwm");
        add_location(div, file, 424, 5, 11021);
      },
      m : function mount(target, anchor) {
        insert_dev(target, div, anchor);
        append_dev(div, t);
      },
      p : function update(ctx, dirty) {
        if (dirty[0] & /*ui_console*/ 64 &&
            t_value !== (t_value = /*entry*/ ctx[30].data + ""))
          set_data_dev(t, t_value);

        if (dirty[0] & /*ui_console*/ 64 &&
            div_class_value !==
                (div_class_value = "consoleline " + /*entry*/ ctx[30].context +
                                   " svelte-16idiwm")) {
          attr_dev(div, "class", div_class_value);
        }
      },
      d : function destroy(detaching) {
        if (detaching)
          detach_dev(div);
      }
    };

    dispatch_dev("SvelteRegisterBlock", {
      block,
      id : create_if_block_2.name,
      type : "if",
      source :
          "(424:4) {#if entry.context == \\\"openocd\\\" && openocd_console_enabled}",
      ctx
    });

    return block;
  }

  // (427:4) {#if entry.context == "telnet" && telnet_console_enabled}
  function create_if_block_1(ctx) {
    let div;
    let t_value = /*entry*/ ctx[30].data + "";
    let t;
    let div_class_value;

    const block = {
      c : function create() {
        div = element("div");
        t = text(t_value);
        attr_dev(div, "class",
                 div_class_value = "consoleline " + /*entry*/ ctx[30].context +
                                   " svelte-16idiwm");
        add_location(div, file, 427, 5, 11158);
      },
      m : function mount(target, anchor) {
        insert_dev(target, div, anchor);
        append_dev(div, t);
      },
      p : function update(ctx, dirty) {
        if (dirty[0] & /*ui_console*/ 64 &&
            t_value !== (t_value = /*entry*/ ctx[30].data + ""))
          set_data_dev(t, t_value);

        if (dirty[0] & /*ui_console*/ 64 &&
            div_class_value !==
                (div_class_value = "consoleline " + /*entry*/ ctx[30].context +
                                   " svelte-16idiwm")) {
          attr_dev(div, "class", div_class_value);
        }
      },
      d : function destroy(detaching) {
        if (detaching)
          detach_dev(div);
      }
    };

    dispatch_dev("SvelteRegisterBlock", {
      block,
      id : create_if_block_1.name,
      type : "if",
      source :
          "(427:4) {#if entry.context == \\\"telnet\\\" && telnet_console_enabled}",
      ctx
    });

    return block;
  }

  // (430:4) {#if entry.context == "fuser" }
  function create_if_block(ctx) {
    let div;
    let t_value = /*entry*/ ctx[30].data + "";
    let t;
    let div_class_value;

    const block = {
      c : function create() {
        div = element("div");
        t = text(t_value);
        attr_dev(div, "class",
                 div_class_value = "consoleline " + /*entry*/ ctx[30].context +
                                   " svelte-16idiwm");
        add_location(div, file, 430, 5, 11269);
      },
      m : function mount(target, anchor) {
        insert_dev(target, div, anchor);
        append_dev(div, t);
      },
      p : function update(ctx, dirty) {
        if (dirty[0] & /*ui_console*/ 64 &&
            t_value !== (t_value = /*entry*/ ctx[30].data + ""))
          set_data_dev(t, t_value);

        if (dirty[0] & /*ui_console*/ 64 &&
            div_class_value !==
                (div_class_value = "consoleline " + /*entry*/ ctx[30].context +
                                   " svelte-16idiwm")) {
          attr_dev(div, "class", div_class_value);
        }
      },
      d : function destroy(detaching) {
        if (detaching)
          detach_dev(div);
      }
    };

    dispatch_dev("SvelteRegisterBlock", {
      block,
      id : create_if_block.name,
      type : "if",
      source : "(430:4) {#if entry.context == \\\"fuser\\\" }",
      ctx
    });

    return block;
  }

  // (420:3) {#each ui_console as entry}
  function create_each_block(ctx) {
    let t0;
    let t1;
    let t2;
    let if_block3_anchor;
    let if_block0 = /*entry*/ ctx[30].context == "uart" &&
                    /*uart_console_enabled*/ ctx[2] && create_if_block_3(ctx);
    let if_block1 = /*entry*/ ctx[30].context == "openocd" &&
                    /*openocd_console_enabled*/ ctx[3] &&
                    create_if_block_2(ctx);
    let if_block2 = /*entry*/ ctx[30].context == "telnet" &&
                    /*telnet_console_enabled*/ ctx[4] && create_if_block_1(ctx);
    let if_block3 =
        /*entry*/ ctx[30].context == "fuser" && create_if_block(ctx);

    const block = {
      c : function create() {
        if (if_block0)
          if_block0.c();
        t0 = space();
        if (if_block1)
          if_block1.c();
        t1 = space();
        if (if_block2)
          if_block2.c();
        t2 = space();
        if (if_block3)
          if_block3.c();
        if_block3_anchor = empty();
      },
      m : function mount(target, anchor) {
        if (if_block0)
          if_block0.m(target, anchor);
        insert_dev(target, t0, anchor);
        if (if_block1)
          if_block1.m(target, anchor);
        insert_dev(target, t1, anchor);
        if (if_block2)
          if_block2.m(target, anchor);
        insert_dev(target, t2, anchor);
        if (if_block3)
          if_block3.m(target, anchor);
        insert_dev(target, if_block3_anchor, anchor);
      },
      p : function update(ctx, dirty) {
        if (/*entry*/ ctx[30].context == "uart" &&
            /*uart_console_enabled*/ ctx[2]) {
          if (if_block0) {
            if_block0.p(ctx, dirty);
          } else {
            if_block0 = create_if_block_3(ctx);
            if_block0.c();
            if_block0.m(t0.parentNode, t0);
          }
        } else if (if_block0) {
          if_block0.d(1);
          if_block0 = null;
        }

        if (/*entry*/ ctx[30].context == "openocd" &&
            /*openocd_console_enabled*/ ctx[3]) {
          if (if_block1) {
            if_block1.p(ctx, dirty);
          } else {
            if_block1 = create_if_block_2(ctx);
            if_block1.c();
            if_block1.m(t1.parentNode, t1);
          }
        } else if (if_block1) {
          if_block1.d(1);
          if_block1 = null;
        }

        if (/*entry*/ ctx[30].context == "telnet" &&
            /*telnet_console_enabled*/ ctx[4]) {
          if (if_block2) {
            if_block2.p(ctx, dirty);
          } else {
            if_block2 = create_if_block_1(ctx);
            if_block2.c();
            if_block2.m(t2.parentNode, t2);
          }
        } else if (if_block2) {
          if_block2.d(1);
          if_block2 = null;
        }

        if (/*entry*/ ctx[30].context == "fuser") {
          if (if_block3) {
            if_block3.p(ctx, dirty);
          } else {
            if_block3 = create_if_block(ctx);
            if_block3.c();
            if_block3.m(if_block3_anchor.parentNode, if_block3_anchor);
          }
        } else if (if_block3) {
          if_block3.d(1);
          if_block3 = null;
        }
      },
      d : function destroy(detaching) {
        if (if_block0)
          if_block0.d(detaching);
        if (detaching)
          detach_dev(t0);
        if (if_block1)
          if_block1.d(detaching);
        if (detaching)
          detach_dev(t1);
        if (if_block2)
          if_block2.d(detaching);
        if (detaching)
          detach_dev(t2);
        if (if_block3)
          if_block3.d(detaching);
        if (detaching)
          detach_dev(if_block3_anchor);
      }
    };

    dispatch_dev("SvelteRegisterBlock", {
      block,
      id : create_each_block.name,
      type : "each",
      source : "(420:3) {#each ui_console as entry}",
      ctx
    });

    return block;
  }

  function create_fragment(ctx) {
    let main;
    let div6;
    let div1;
    let div0;
    let table;
    let tr0;
    let td0;
    let td1;
    let t1_value = /*grid*/ ctx[7].mcu + "";
    let t1;
    let td2;
    let t2_value = /*grid*/ ctx[7].mcustatus + "";
    let t2;
    let t3;
    let tr1;
    let td3;
    let td4;
    let t5_value = /*grid*/ ctx[7].hwcfg + "";
    let t5;
    let td5;
    let t6_value = /*grid*/ ctx[7].hwcfgstatus + "";
    let t6;
    let t7;
    let tr2;
    let td6;
    let td7;
    let t9_value = /*grid*/ ctx[7].model + "";
    let t9;
    let t10;
    let tr3;
    let td8;
    let td9;
    let t12_value = /*grid*/ ctx[7].serialno[0] + "";
    let t12;
    let td10;
    let t13_value = /*grid*/ ctx[7].serialno[1] + "";
    let t13;
    let t14;
    let tr4;
    let td11;
    let td12;
    let t15_value = /*grid*/ ctx[7].serialno[2] + "";
    let t15;
    let td13;
    let t16_value = /*grid*/ ctx[7].serialno[3] + "";
    let t16;
    let t17;
    let div2;
    let t18;
    let div3;
    let t19;
    let div4;
    let t20;
    let div5;
    let t21;
    let div8;
    let div7;
    let t22;
    let div9;
    let input0;
    let t23;
    let input1;
    let t24;
    let input2;
    let t25;
    let input3;
    let t26;
    let input4;
    let t27;
    let div10;
    let input5;
    let t28;
    let input6;
    let t29;
    let div11;
    let input7;
    let t30;
    let input8;
    let t31;
    let input9;
    let label0;
    let t33;
    let input10;
    let label1;
    let t35;
    let input11;
    let label2;
    let t37;
    let input12;
    let label3;
    let t39;
    let div12;
    let t40;
    let input13;
    let t41;
    let input14;
    let t42;
    let input15;
    let t43;
    let div13;
    let t44;
    let input16;
    let t45;
    let input17;
    let t46;
    let input18;
    let t47;
    let input19;
    let t48;
    let div14;
    let t49;
    let input20;
    let mounted;
    let dispose;
    let each_value_4 = /*mcu_pins*/ ctx[8][0];
    validate_each_argument(each_value_4);
    let each_blocks_4 = [];

    for (let i = 0; i < each_value_4.length; i += 1) {
      each_blocks_4[i] =
          create_each_block_4(get_each_context_4(ctx, each_value_4, i));
    }

    let each_value_3 = /*mcu_pins*/ ctx[8][1];
    validate_each_argument(each_value_3);
    let each_blocks_3 = [];

    for (let i = 0; i < each_value_3.length; i += 1) {
      each_blocks_3[i] =
          create_each_block_3(get_each_context_3(ctx, each_value_3, i));
    }

    let each_value_2 = /*mcu_pins*/ ctx[8][2];
    validate_each_argument(each_value_2);
    let each_blocks_2 = [];

    for (let i = 0; i < each_value_2.length; i += 1) {
      each_blocks_2[i] =
          create_each_block_2(get_each_context_2(ctx, each_value_2, i));
    }

    let each_value_1 = /*mcu_pins*/ ctx[8][3];
    validate_each_argument(each_value_1);
    let each_blocks_1 = [];

    for (let i = 0; i < each_value_1.length; i += 1) {
      each_blocks_1[i] =
          create_each_block_1(get_each_context_1(ctx, each_value_1, i));
    }

    let each_value = /*ui_console*/ ctx[6];
    validate_each_argument(each_value);
    let each_blocks = [];

    for (let i = 0; i < each_value.length; i += 1) {
      each_blocks[i] = create_each_block(get_each_context(ctx, each_value, i));
    }

    const block = {
      c : function create() {
        main = element("main");
        div6 = element("div");
        div1 = element("div");
        div0 = element("div");
        table = element("table");
        tr0 = element("tr");
        td0 = element("td");
        td0.textContent = "MCU: ";
        td1 = element("td");
        t1 = text(t1_value);
        td2 = element("td");
        t2 = text(t2_value);
        t3 = space();
        tr1 = element("tr");
        td3 = element("td");
        td3.textContent = "HWCFG:";
        td4 = element("td");
        t5 = text(t5_value);
        td5 = element("td");
        t6 = text(t6_value);
        t7 = space();
        tr2 = element("tr");
        td6 = element("td");
        td6.textContent = "Model:";
        td7 = element("td");
        t9 = text(t9_value);
        t10 = space();
        tr3 = element("tr");
        td8 = element("td");
        td8.textContent = "S/N:";
        td9 = element("td");
        t12 = text(t12_value);
        td10 = element("td");
        t13 = text(t13_value);
        t14 = space();
        tr4 = element("tr");
        td11 = element("td");
        td12 = element("td");
        t15 = text(t15_value);
        td13 = element("td");
        t16 = text(t16_value);
        t17 = space();
        div2 = element("div");

        for (let i = 0; i < each_blocks_4.length; i += 1) {
          each_blocks_4[i].c();
        }

        t18 = space();
        div3 = element("div");

        for (let i = 0; i < each_blocks_3.length; i += 1) {
          each_blocks_3[i].c();
        }

        t19 = space();
        div4 = element("div");

        for (let i = 0; i < each_blocks_2.length; i += 1) {
          each_blocks_2[i].c();
        }

        t20 = space();
        div5 = element("div");

        for (let i = 0; i < each_blocks_1.length; i += 1) {
          each_blocks_1[i].c();
        }

        t21 = space();
        div8 = element("div");
        div7 = element("div");

        for (let i = 0; i < each_blocks.length; i += 1) {
          each_blocks[i].c();
        }

        t22 = space();
        div9 = element("div");
        input0 = element("input");
        t23 = space();
        input1 = element("input");
        t24 = space();
        input2 = element("input");
        t25 = space();
        input3 = element("input");
        t26 = space();
        input4 = element("input");
        t27 = space();
        div10 = element("div");
        input5 = element("input");
        t28 = space();
        input6 = element("input");
        t29 = space();
        div11 = element("div");
        input7 = element("input");
        t30 = space();
        input8 = element("input");
        t31 = space();
        input9 = element("input");
        label0 = element("label");
        label0.textContent = "UART";
        t33 = space();
        input10 = element("input");
        label1 = element("label");
        label1.textContent = "OpenOCD";
        t35 = space();
        input11 = element("input");
        label2 = element("label");
        label2.textContent = "Telnet";
        t37 = space();
        input12 = element("input");
        label3 = element("label");
        label3.textContent = "Autosend";
        t39 = space();
        div12 = element("div");
        t40 = text("Chip: \n\t\t");
        input13 = element("input");
        t41 = space();
        input14 = element("input");
        t42 = space();
        input15 = element("input");
        t43 = space();
        div13 = element("div");
        t44 = text("Bootloader: \n\t\t");
        input16 = element("input");
        t45 = space();
        input17 = element("input");
        t46 = space();
        input18 = element("input");
        t47 = space();
        input19 = element("input");
        t48 = space();
        div14 = element("div");
        t49 = text("Firmware: \n\t\t");
        input20 = element("input");
        add_location(td0, file, 366, 6, 9724);
        add_location(td1, file, 366, 20, 9738);
        add_location(td2, file, 366, 39, 9757);
        add_location(tr0, file, 365, 5, 9713);
        add_location(td3, file, 370, 6, 9812);
        add_location(td4, file, 370, 21, 9827);
        add_location(td5, file, 370, 42, 9848);
        add_location(tr1, file, 369, 5, 9801);
        add_location(td6, file, 373, 6, 9903);
        attr_dev(td7, "colspan", "2");
        add_location(td7, file, 373, 21, 9918);
        add_location(tr2, file, 372, 5, 9892);
        add_location(td8, file, 377, 6, 9980);
        add_location(td9, file, 377, 19, 9993);
        add_location(td10, file, 377, 46, 10020);
        add_location(tr3, file, 376, 5, 9969);
        add_location(td11, file, 380, 6, 10075);
        add_location(td12, file, 380, 15, 10084);
        add_location(td13, file, 380, 42, 10111);
        add_location(tr4, file, 379, 5, 10064);
        attr_dev(table, "border", "1");
        attr_dev(table, "width", "300px");
        add_location(table, file, 363, 4, 9674);
        attr_dev(div0, "class", "chip_info svelte-16idiwm");
        add_location(div0, file, 362, 6, 9646);
        attr_dev(div1, "class", "chip svelte-16idiwm");
        add_location(div1, file, 360, 2, 9620);
        attr_dev(div2, "class", "side rot0 svelte-16idiwm");
        add_location(div2, file, 389, 2, 10188);
        attr_dev(div3, "class", "side rot90 svelte-16idiwm");
        add_location(div3, file, 394, 2, 10318);
        attr_dev(div4, "class", "side rot180 svelte-16idiwm");
        add_location(div4, file, 401, 2, 10449);
        attr_dev(div5, "class", "side rot270 svelte-16idiwm");
        add_location(div5, file, 406, 2, 10581);
        attr_dev(div6, "class", "boundary_check container svelte-16idiwm");
        add_location(div6, file, 358, 1, 9578);
        attr_dev(div7, "class", "serial_console svelte-16idiwm");
        add_location(div7, file, 418, 2, 10759);
        attr_dev(div8, "class", "serial_container svelte-16idiwm");
        add_location(div8, file, 417, 1, 10726);
        attr_dev(input0, "type", "button");
        input0.value = "Reset Ports";
        add_location(input0, file, 442, 2, 11386);
        attr_dev(input1, "type", "button");
        input1.value = "OpenOCD Start";
        add_location(input1, file, 444, 2, 11453);
        attr_dev(input2, "type", "button");
        input2.value = "OpenOCD Stop";
        add_location(input2, file, 445, 2, 11524);
        attr_dev(input3, "type", "button");
        input3.value = "Telnet Start";
        add_location(input3, file, 446, 2, 11593);
        attr_dev(input4, "type", "button");
        input4.value = "Telnet Stop";
        add_location(input4, file, 447, 2, 11662);
        add_location(div9, file, 438, 1, 11372);
        attr_dev(input5, "type", "text");
        attr_dev(input5, "placeholder", "UART command");
        add_location(input5, file, 450, 2, 11744);
        attr_dev(input6, "type", "button");
        input6.value = "Send";
        add_location(input6, file, 451, 2, 11824);
        add_location(div10, file, 449, 1, 11736);
        attr_dev(input7, "type", "text");
        attr_dev(input7, "placeholder", "Telnet command");
        add_location(input7, file, 455, 2, 11898);
        attr_dev(input8, "type", "button");
        input8.value = "Send";
        add_location(input8, file, 456, 2, 12006);
        attr_dev(input9, "name", "uart_enable");
        attr_dev(input9, "id", "uart_enable");
        attr_dev(input9, "type", "checkbox");
        add_location(input9, file, 458, 2, 12067);
        attr_dev(label0, "for", "uart_enable");
        attr_dev(label0, "class", "svelte-16idiwm");
        add_location(label0, file, 458, 97, 12162);
        attr_dev(input10, "name", "openocd_enable");
        attr_dev(input10, "id", "openocd_enable");
        attr_dev(input10, "type", "checkbox");
        add_location(input10, file, 459, 2, 12202);
        attr_dev(label1, "for", "openocd_enable");
        attr_dev(label1, "class", "svelte-16idiwm");
        add_location(label1, file, 459, 106, 12306);
        attr_dev(input11, "name", "telnet_enable");
        attr_dev(input11, "id", "telnet_enable");
        attr_dev(input11, "type", "checkbox");
        add_location(input11, file, 460, 2, 12352);
        attr_dev(label2, "for", "telnet_enable");
        attr_dev(label2, "class", "svelte-16idiwm");
        add_location(label2, file, 460, 103, 12453);
        attr_dev(input12, "name", "telnet_autosend");
        attr_dev(input12, "id", "telnet_autosend");
        attr_dev(input12, "type", "checkbox");
        add_location(input12, file, 461, 2, 12497);
        attr_dev(label3, "for", "telnet_autosend");
        attr_dev(label3, "class", "svelte-16idiwm");
        add_location(label3, file, 461, 100, 12595);
        add_location(div11, file, 454, 1, 11890);
        attr_dev(input13, "type", "button");
        input13.value = "reset";
        add_location(input13, file, 466, 2, 12669);
        attr_dev(input14, "type", "button");
        input14.value = "reset init";
        add_location(input14, file, 467, 2, 12796);
        attr_dev(input15, "type", "button");
        input15.value = "erase";
        add_location(input15, file, 468, 2, 12933);
        add_location(div12, file, 464, 1, 12652);
        attr_dev(input16, "type", "button");
        input16.value = "check";
        add_location(input16, file, 473, 2, 13104);
        attr_dev(input17, "type", "button");
        input17.value = "lock";
        add_location(input17, file, 474, 2, 13244);
        attr_dev(input18, "type", "button");
        input18.value = "unlock";
        add_location(input18, file, 475, 2, 13389);
        attr_dev(input19, "type", "button");
        input19.value = "install";
        add_location(input19, file, 476, 2, 13532);
        add_location(div13, file, 471, 1, 13081);
        attr_dev(input20, "type", "button");
        input20.value = "install";
        add_location(input20, file, 481, 2, 13750);
        add_location(div14, file, 479, 1, 13729);
        attr_dev(main, "class", "svelte-16idiwm");
        add_location(main, file, 356, 0, 9569);
      },
      l : function claim(nodes) {
        throw new Error(
            "options.hydrate only works if the component was compiled with the `hydratable: true` option");
      },
      m : function mount(target, anchor) {
        insert_dev(target, main, anchor);
        append_dev(main, div6);
        append_dev(div6, div1);
        append_dev(div1, div0);
        append_dev(div0, table);
        append_dev(table, tr0);
        append_dev(tr0, td0);
        append_dev(tr0, td1);
        append_dev(td1, t1);
        append_dev(tr0, td2);
        append_dev(td2, t2);
        append_dev(table, t3);
        append_dev(table, tr1);
        append_dev(tr1, td3);
        append_dev(tr1, td4);
        append_dev(td4, t5);
        append_dev(tr1, td5);
        append_dev(td5, t6);
        append_dev(table, t7);
        append_dev(table, tr2);
        append_dev(tr2, td6);
        append_dev(tr2, td7);
        append_dev(td7, t9);
        append_dev(table, t10);
        append_dev(table, tr3);
        append_dev(tr3, td8);
        append_dev(tr3, td9);
        append_dev(td9, t12);
        append_dev(tr3, td10);
        append_dev(td10, t13);
        append_dev(table, t14);
        append_dev(table, tr4);
        append_dev(tr4, td11);
        append_dev(tr4, td12);
        append_dev(td12, t15);
        append_dev(tr4, td13);
        append_dev(td13, t16);
        append_dev(div6, t17);
        append_dev(div6, div2);

        for (let i = 0; i < each_blocks_4.length; i += 1) {
          each_blocks_4[i].m(div2, null);
        }

        append_dev(div6, t18);
        append_dev(div6, div3);

        for (let i = 0; i < each_blocks_3.length; i += 1) {
          each_blocks_3[i].m(div3, null);
        }

        append_dev(div6, t19);
        append_dev(div6, div4);

        for (let i = 0; i < each_blocks_2.length; i += 1) {
          each_blocks_2[i].m(div4, null);
        }

        append_dev(div6, t20);
        append_dev(div6, div5);

        for (let i = 0; i < each_blocks_1.length; i += 1) {
          each_blocks_1[i].m(div5, null);
        }

        append_dev(main, t21);
        append_dev(main, div8);
        append_dev(div8, div7);

        for (let i = 0; i < each_blocks.length; i += 1) {
          each_blocks[i].m(div7, null);
        }

        append_dev(main, t22);
        append_dev(main, div9);
        append_dev(div9, input0);
        append_dev(div9, t23);
        append_dev(div9, input1);
        append_dev(div9, t24);
        append_dev(div9, input2);
        append_dev(div9, t25);
        append_dev(div9, input3);
        append_dev(div9, t26);
        append_dev(div9, input4);
        append_dev(main, t27);
        append_dev(main, div10);
        append_dev(div10, input5);
        set_input_value(input5, /*uart_input_field*/ ctx[0]);
        append_dev(div10, t28);
        append_dev(div10, input6);
        append_dev(main, t29);
        append_dev(main, div11);
        append_dev(div11, input7);
        set_input_value(input7, /*telnet_input_field*/ ctx[1]);
        append_dev(div11, t30);
        append_dev(div11, input8);
        append_dev(div11, t31);
        append_dev(div11, input9);
        input9.checked = /*uart_console_enabled*/ ctx[2];
        append_dev(div11, label0);
        append_dev(div11, t33);
        append_dev(div11, input10);
        input10.checked = /*openocd_console_enabled*/ ctx[3];
        append_dev(div11, label1);
        append_dev(div11, t35);
        append_dev(div11, input11);
        input11.checked = /*telnet_console_enabled*/ ctx[4];
        append_dev(div11, label2);
        append_dev(div11, t37);
        append_dev(div11, input12);
        input12.checked = /*telnet_autosend*/ ctx[5];
        append_dev(div11, label3);
        append_dev(main, t39);
        append_dev(main, div12);
        append_dev(div12, t40);
        append_dev(div12, input13);
        append_dev(div12, t41);
        append_dev(div12, input14);
        append_dev(div12, t42);
        append_dev(div12, input15);
        append_dev(main, t43);
        append_dev(main, div13);
        append_dev(div13, t44);
        append_dev(div13, input16);
        append_dev(div13, t45);
        append_dev(div13, input17);
        append_dev(div13, t46);
        append_dev(div13, input18);
        append_dev(div13, t47);
        append_dev(div13, input19);
        append_dev(main, t48);
        append_dev(main, div14);
        append_dev(div14, t49);
        append_dev(div14, input20);

        if (!mounted) {
          dispose = [
            listen_dev(input0, "click", fuser_kill, false, false, false),
            listen_dev(input1, "click", openocd_start, false, false, false),
            listen_dev(input2, "click", openocd_stop, false, false, false),
            listen_dev(input3, "click", telnet_start, false, false, false),
            listen_dev(input4, "click", telnet_stop, false, false, false),
            listen_dev(input5, "input", /*input5_input_handler*/ ctx[13]),
            listen_dev(input6, "click", /*uart_send*/ ctx[11], false, false,
                       false),
            listen_dev(input7, "keyup", /*keyup_telnet*/ ctx[9], false, false,
                       false),
            listen_dev(input7, "input", /*input7_input_handler*/ ctx[14]),
            listen_dev(input8, "click", /*telnet_send*/ ctx[10], false, false,
                       false),
            listen_dev(input9, "change", /*input9_change_handler*/ ctx[15]),
            listen_dev(input10, "change", /*input10_change_handler*/ ctx[16]),
            listen_dev(input11, "change", /*input11_change_handler*/ ctx[17]),
            listen_dev(input12, "change", /*input12_change_handler*/ ctx[18]),
            listen_dev(input13, "click", /*click_handler*/ ctx[19], false,
                       false, false),
            listen_dev(input14, "click", /*click_handler_1*/ ctx[20], false,
                       false, false),
            listen_dev(input15, "click", /*click_handler_2*/ ctx[21], false,
                       false, false),
            listen_dev(input16, "click", /*click_handler_3*/ ctx[22], false,
                       false, false),
            listen_dev(input17, "click", /*click_handler_4*/ ctx[23], false,
                       false, false),
            listen_dev(input18, "click", /*click_handler_5*/ ctx[24], false,
                       false, false),
            listen_dev(input19, "click", /*click_handler_6*/ ctx[25], false,
                       false, false),
            listen_dev(input20, "click", /*click_handler_7*/ ctx[26], false,
                       false, false)
          ];

          mounted = true;
        }
      },
      p : function update(ctx, dirty) {
        if (dirty[0] & /*grid*/ 128 &&
            t1_value !== (t1_value = /*grid*/ ctx[7].mcu + ""))
          set_data_dev(t1, t1_value);
        if (dirty[0] & /*grid*/ 128 &&
            t2_value !== (t2_value = /*grid*/ ctx[7].mcustatus + ""))
          set_data_dev(t2, t2_value);
        if (dirty[0] & /*grid*/ 128 &&
            t5_value !== (t5_value = /*grid*/ ctx[7].hwcfg + ""))
          set_data_dev(t5, t5_value);
        if (dirty[0] & /*grid*/ 128 &&
            t6_value !== (t6_value = /*grid*/ ctx[7].hwcfgstatus + ""))
          set_data_dev(t6, t6_value);
        if (dirty[0] & /*grid*/ 128 &&
            t9_value !== (t9_value = /*grid*/ ctx[7].model + ""))
          set_data_dev(t9, t9_value);
        if (dirty[0] & /*grid*/ 128 &&
            t12_value !== (t12_value = /*grid*/ ctx[7].serialno[0] + ""))
          set_data_dev(t12, t12_value);
        if (dirty[0] & /*grid*/ 128 &&
            t13_value !== (t13_value = /*grid*/ ctx[7].serialno[1] + ""))
          set_data_dev(t13, t13_value);
        if (dirty[0] & /*grid*/ 128 &&
            t15_value !== (t15_value = /*grid*/ ctx[7].serialno[2] + ""))
          set_data_dev(t15, t15_value);
        if (dirty[0] & /*grid*/ 128 &&
            t16_value !== (t16_value = /*grid*/ ctx[7].serialno[3] + ""))
          set_data_dev(t16, t16_value);

        if (dirty[0] & /*mcu_pins*/ 256) {
          each_value_4 = /*mcu_pins*/ ctx[8][0];
          validate_each_argument(each_value_4);
          let i;

          for (i = 0; i < each_value_4.length; i += 1) {
            const child_ctx = get_each_context_4(ctx, each_value_4, i);

            if (each_blocks_4[i]) {
              each_blocks_4[i].p(child_ctx, dirty);
            } else {
              each_blocks_4[i] = create_each_block_4(child_ctx);
              each_blocks_4[i].c();
              each_blocks_4[i].m(div2, null);
            }
          }

          for (; i < each_blocks_4.length; i += 1) {
            each_blocks_4[i].d(1);
          }

          each_blocks_4.length = each_value_4.length;
        }

        if (dirty[0] & /*mcu_pins*/ 256) {
          each_value_3 = /*mcu_pins*/ ctx[8][1];
          validate_each_argument(each_value_3);
          let i;

          for (i = 0; i < each_value_3.length; i += 1) {
            const child_ctx = get_each_context_3(ctx, each_value_3, i);

            if (each_blocks_3[i]) {
              each_blocks_3[i].p(child_ctx, dirty);
            } else {
              each_blocks_3[i] = create_each_block_3(child_ctx);
              each_blocks_3[i].c();
              each_blocks_3[i].m(div3, null);
            }
          }

          for (; i < each_blocks_3.length; i += 1) {
            each_blocks_3[i].d(1);
          }

          each_blocks_3.length = each_value_3.length;
        }

        if (dirty[0] & /*mcu_pins*/ 256) {
          each_value_2 = /*mcu_pins*/ ctx[8][2];
          validate_each_argument(each_value_2);
          let i;

          for (i = 0; i < each_value_2.length; i += 1) {
            const child_ctx = get_each_context_2(ctx, each_value_2, i);

            if (each_blocks_2[i]) {
              each_blocks_2[i].p(child_ctx, dirty);
            } else {
              each_blocks_2[i] = create_each_block_2(child_ctx);
              each_blocks_2[i].c();
              each_blocks_2[i].m(div4, null);
            }
          }

          for (; i < each_blocks_2.length; i += 1) {
            each_blocks_2[i].d(1);
          }

          each_blocks_2.length = each_value_2.length;
        }

        if (dirty[0] & /*mcu_pins*/ 256) {
          each_value_1 = /*mcu_pins*/ ctx[8][3];
          validate_each_argument(each_value_1);
          let i;

          for (i = 0; i < each_value_1.length; i += 1) {
            const child_ctx = get_each_context_1(ctx, each_value_1, i);

            if (each_blocks_1[i]) {
              each_blocks_1[i].p(child_ctx, dirty);
            } else {
              each_blocks_1[i] = create_each_block_1(child_ctx);
              each_blocks_1[i].c();
              each_blocks_1[i].m(div5, null);
            }
          }

          for (; i < each_blocks_1.length; i += 1) {
            each_blocks_1[i].d(1);
          }

          each_blocks_1.length = each_value_1.length;
        }

        if (dirty[0] & /*ui_console, telnet_console_enabled,
                          openocd_console_enabled, uart_console_enabled*/
            92) {
          each_value = /*ui_console*/ ctx[6];
          validate_each_argument(each_value);
          let i;

          for (i = 0; i < each_value.length; i += 1) {
            const child_ctx = get_each_context(ctx, each_value, i);

            if (each_blocks[i]) {
              each_blocks[i].p(child_ctx, dirty);
            } else {
              each_blocks[i] = create_each_block(child_ctx);
              each_blocks[i].c();
              each_blocks[i].m(div7, null);
            }
          }

          for (; i < each_blocks.length; i += 1) {
            each_blocks[i].d(1);
          }

          each_blocks.length = each_value.length;
        }

        if (dirty[0] & /*uart_input_field*/ 1 &&
            input5.value !== /*uart_input_field*/ ctx[0]) {
          set_input_value(input5, /*uart_input_field*/ ctx[0]);
        }

        if (dirty[0] & /*telnet_input_field*/ 2 &&
            input7.value !== /*telnet_input_field*/ ctx[1]) {
          set_input_value(input7, /*telnet_input_field*/ ctx[1]);
        }

        if (dirty[0] & /*uart_console_enabled*/ 4) {
          input9.checked = /*uart_console_enabled*/ ctx[2];
        }

        if (dirty[0] & /*openocd_console_enabled*/ 8) {
          input10.checked = /*openocd_console_enabled*/ ctx[3];
        }

        if (dirty[0] & /*telnet_console_enabled*/ 16) {
          input11.checked = /*telnet_console_enabled*/ ctx[4];
        }

        if (dirty[0] & /*telnet_autosend*/ 32) {
          input12.checked = /*telnet_autosend*/ ctx[5];
        }
      },
      i : noop,
      o : noop,
      d : function destroy(detaching) {
        if (detaching)
          detach_dev(main);
        destroy_each(each_blocks_4, detaching);
        destroy_each(each_blocks_3, detaching);
        destroy_each(each_blocks_2, detaching);
        destroy_each(each_blocks_1, detaching);
        destroy_each(each_blocks, detaching);
        mounted = false;
        run_all(dispose);
      }
    };

    dispatch_dev("SvelteRegisterBlock", {
      block,
      id : create_fragment.name,
      type : "component",
      source : "",
      ctx
    });

    return block;
  }

  async function fuser_kill() {
    try {
      const serial = await fetch("/api/fuser/kill", {
                       method : "GET"
                     }).then(res => res.json());
    } catch (error) {

    } // console.log(serial)
  }

  async function openocd_start() {
    try {
      const serial = await fetch("/api/openocd/start", {
                       method : "GET"
                     }).then(res => res.json());
    } catch (error) {

    } // console.log(serial)
  }

  async function openocd_stop() {
    try {
      const serial = await fetch("/api/openocd/stop", {
                       method : "GET"
                     }).then(res => res.json());
    } catch (error) {

    } // console.log(serial)
  }

  async function telnet_start() {
    try {
      const serial = await fetch("/api/telnet/start", {
                       method : "GET"
                     }).then(res => res.json());
    } catch (error) {

    } // console.log(serial)
  }

  async function telnet_stop() {
    try {
      const serial = await fetch("/api/telnet/stop", {
                       method : "GET"
                     }).then(res => res.json());
    } catch (error) {

    } // console.log(serial)
  }

  function instance($$self, $$props, $$invalidate) {
    let {$$slots : slots = {}, $$scope} = $$props;
    validate_slots("App", slots, []);
    let {name} = $$props;
    let uart_input_field = "";
    let telnet_input_field = "";
    let telnet_history = [];
    let telnet_history_index = 0;
    let uart_console_enabled = true;
    let openocd_console_enabled = true;
    let telnet_console_enabled = true;
    let telnet_autosend = true;

    function keyup_telnet(e) {
      if (e.keyCode === 13) {
        e.preventDefault();
        telnet_send();
      }

      if (e.keyCode === 38) {
        if (telnet_history_index < telnet_history.length) {
          telnet_history_index++;
        }

        $$invalidate(1, telnet_input_field =
                            telnet_history[telnet_history.length -
                                           (telnet_history_index - 1) - 1]);
      }

      if (e.keyCode === 40) {
        if (telnet_history_index > 0) {
          telnet_history_index--;
        }

        $$invalidate(
            1,
            telnet_input_field =
                telnet_history[telnet_history.length - telnet_history_index]);
      }
    }

    async function telnet_send() {
      telnet_history.push(telnet_input_field);
      telnet_history_index = 0;

      try {
        const serial = await fetch("/api/telnet/send", {
                         method : "POST",
                         body : JSON.stringify({data : telnet_input_field}),
                         headers : {
                           "Accept" : "application/json",
                           "Content-Type" : "application/json"
                         }
                       }).then(res => {
          console.log(res);
          $$invalidate(1, telnet_input_field = "");
          return;
        });
      } catch (error) {
      }
    }

    async function uart_send() {
      console.log(uart_input_field);

      try {
        const serial = await fetch("/api/uart/send", {
                         method : "POST",
                         body : JSON.stringify({data : uart_input_field}),
                         headers : {
                           "Accept" : "application/json",
                           "Content-Type" : "application/json"
                         }
                       }).then(res => {
          console.log(res);
          $$invalidate(0, uart_input_field = "");
          return;
        });
      } catch (error) {
      }
    }

    async function consolePoll() {
      let serial = [];

      try {
        serial = await fetch("/api/console", {
                   method : "GET"
                 }).then(res => res.json());

        serial.forEach(element => {
          // var str = String.fromCharCode.apply(null, element.data);
          console.log(element);

          $$invalidate(6, ui_console = [ element, ...ui_console ]);

          // ui_console.data = [...ui_console.data, element.data];
          // ui_console.context = [...ui_console.context, element.context];
          let str = element.data;

          if (str.startsWith("test")) {
            if (str.startsWith("test.boundary.")) {
              var test_result = str.substring(16, 16 + 25);
              console.log("Boundary Test Result is Here!");
              console.log(parseInt(str.substring(14, 15)));
              console.log(test_result);
              var indices = [];

              for (var i = 0; i < test_result.length; i++) {
                if (test_result[i] === "1")
                  indices.push(i);
              }

              console.log(indices);

              for (var i = 0; i < indices.length; i++) {
                let side = parseInt(str.substring(14, 15));
                $$invalidate(8, mcu_pins[side][indices[i]].class += "pinerror",
                             mcu_pins);
              }
            } else if (str.startsWith("test.hwcfg.")) {
              $$invalidate(7,
                           grid.hwcfg = parseInt(str.substring(11, str.length)),
                           grid);

              if (grid.hwcfg == 192) {
                $$invalidate(7, grid.model = "EN16 RevA", grid);
                $$invalidate(7, grid.hwcfgstatus = "OK", grid);
              }
            } else if (str.startsWith("test.serialno.")) {
              $$invalidate(
                  7, grid.serialno = str.substring(14, str.length).split(" "),
                  grid);
            } else if (str.startsWith("test.mcu.")) {
              $$invalidate(7, grid.mcu = str.split(".")[2], grid);

              if (grid.mcu == "ATSAMD51N20A") {
                $$invalidate(7, grid.mcustatus = "OK", grid);
              }
            }
          }
        });
      } catch (error) {
      }
    }

    onMount(() => { setInterval(consolePoll, 1000); });

    let ui_console = [
      {context : "uart", data : "123"}, {context : "telnet", data : "456"}
    ];

    let grid = {
      hwcfg : "???",
      hwcfgstatus : "?",
      mcu : "?",
      mcustatus : "?",
      model : "???",
      serialno : [ "?", "?", "?", "?" ]
    };

    let mcu_pins = [
      [
        {number : "1", function : "1", class : ""},
        {number : "2", function : "2", class : ""},
        {number : "3", function : "3", class : ""},
        {number : "4", function : "4", class : ""},
        {number : "5", function : "5", class : ""},
        {number : "6", function : "6", class : ""},
        {number : "7", function : "7", class : ""},
        {number : "8", function : "8", class : ""},
        {number : "9", function : "9", class : ""},
        {number : "10", function : "10", class : ""},
        {number : "11", function : "GND", class : "pingnd"},
        {number : "12", function : "VDD", class : "pinpwr"},
        {number : "13", function : "13", class : ""},
        {number : "14", function : "SYNC2", class : ""},
        {number : "15", function : "WEST TX", class : ""},
        {number : "16", function : "WEST RX", class : ""},
        {number : "17", function : "17", class : ""},
        {number : "18", function : "18", class : ""},
        {number : "19", function : "19", class : ""},
        {number : "20", function : "20", class : ""},
        {number : "21", function : "21", class : ""},
        {number : "22", function : "22", class : ""},
        {number : "23", function : "23", class : ""},
        {number : "24", function : "GND", class : "pingnd"},
        {number : "25", function : "VDD", class : "pinpwr"}
      ],
      [
        {number : "26", function : "QSPI IO 0", class : ""},
        {number : "27", function : "QSPI IO 1", class : ""},
        {number : "28", function : "QSPI IO 2", class : ""},
        {number : "29", function : "QSPI IO 3", class : ""},
        {number : "30", function : "VDD", class : "pinpwr"},
        {number : "31", function : "GND", class : "pingnd"},
        {number : "32", function : "QSPI SCK", class : ""},
        {number : "33", function : "QSPI CE", class : ""},
        {number : "34", function : "34", class : ""},
        {number : "35", function : "HWC SH", class : ""},
        {number : "36", function : "HWC CLK", class : ""},
        {number : "37", function : "HWC DAT", class : ""},
        {number : "38", function : "GND", class : "pingnd"},
        {number : "39", function : "VDD", class : "pinpwr"},
        {number : "40", function : "40", class : ""},
        {number : "41", function : "MAPMODE", class : ""},
        {number : "42", function : "SOUTH RX", class : ""},
        {number : "43", function : "SOUTH TX", class : ""},
        {number : "44", function : "UI PWR EN", class : ""},
        {number : "45", function : "45", class : ""},
        {number : "46", function : "46", class : ""},
        {number : "47", function : "47", class : ""},
        {number : "48", function : "48", class : ""},
        {number : "49", function : "49", class : ""},
        {number : "50", function : "GND", class : "pingnd"}
      ],
      [
        {number : "51", function : "VDD", class : "pinpwr"},
        {number : "52", function : "52", class : ""},
        {number : "53", function : "53", class : ""},
        {number : "54", function : "54", class : ""},
        {number : "55", function : "55", class : ""},
        {number : "56", function : "EAST RX", class : ""},
        {number : "57", function : "EAST TX", class : ""},
        {number : "58", function : "SYNC1", class : ""},
        {number : "59", function : "59", class : ""},
        {number : "60", function : "60", class : ""},
        {number : "61", function : "61", class : ""},
        {number : "62", function : "62", class : "pingnd"},
        {number : "63", function : "63", class : "pinpwr"},
        {number : "64", function : "64", class : ""},
        {number : "65", function : "65", class : ""},
        {number : "66", function : "66", class : ""},
        {number : "67", function : "67", class : ""},
        {number : "68", function : "68", class : ""},
        {number : "69", function : "69", class : ""},
        {number : "70", function : "70", class : ""},
        {number : "71", function : "71", class : ""},
        {number : "72", function : "SYS SCL", class : ""},
        {number : "73", function : "SYS SDA", class : ""},
        {number : "74", function : "USB DN", class : ""},
        {number : "75", function : "USB DP", class : ""}
      ],
      [
        {number : "76", function : "GND", class : "pingnd"},
        {number : "77", function : "PWR", class : "pinpwr"},
        {number : "78", function : "78", class : ""},
        {number : "79", function : "79", class : ""},
        {number : "80", function : "DBG RX", class : ""},
        {number : "81", function : "DBG TX", class : ""},
        {number : "82", function : "SYS INT 0", class : ""},
        {number : "83", function : "83", class : ""},
        {number : "84", function : "84", class : ""},
        {number : "85", function : "NORTH TX", class : ""},
        {number : "86", function : "NORTH RX", class : ""},
        {number : "87", function : "87", class : ""},
        {number : "88", function : "RESET", class : "pinrst"},
        {number : "89", function : "CORE", class : "pinpwr"},
        {number : "90", function : "GND", class : "pingnd"},
        {number : "91", function : "VSW", class : "pinpwr"},
        {number : "92", function : "VDD", class : "pinpwr"},
        {number : "93", function : "SWCLK", class : ""},
        {number : "94", function : "SWDIO", class : ""},
        {number : "95", function : "SWO LED", class : ""},
        {number : "96", function : "96", class : ""},
        {number : "97", function : "97", class : ""},
        {number : "98", function : "98", class : ""},
        {number : "99", function : "99", class : ""},
        {number : "100", function : "100", class : ""}
      ]
    ];

    const writable_props = [ "name" ];

    Object.keys($$props).forEach(key => {
      if (!~writable_props.indexOf(key) && key.slice(0, 2) !== "$$")
        console_1.warn(`<App> was created with unknown prop '${key}'`);
    });

    function input5_input_handler() {
      uart_input_field = this.value;
      $$invalidate(0, uart_input_field);
    }

    function input7_input_handler() {
      telnet_input_field = this.value;
      $$invalidate(1, telnet_input_field);
    }

    function input9_change_handler() {
      uart_console_enabled = this.checked;
      $$invalidate(2, uart_console_enabled);
    }

    function input10_change_handler() {
      openocd_console_enabled = this.checked;
      $$invalidate(3, openocd_console_enabled);
    }

    function input11_change_handler() {
      telnet_console_enabled = this.checked;
      $$invalidate(4, telnet_console_enabled);
    }

    function input12_change_handler() {
      telnet_autosend = this.checked;
      $$invalidate(5, telnet_autosend);
    }

    const click_handler = function() {
      $$invalidate(1, telnet_input_field = "reset");
      if (telnet_autosend)
        telnet_send();
    };

    const click_handler_1 = function() {
      $$invalidate(1, telnet_input_field = "reset init");
      if (telnet_autosend)
        telnet_send();
    };

    const click_handler_2 = function() {
      $$invalidate(1, telnet_input_field = "atsame5 chip-erase");
      if (telnet_autosend)
        telnet_send();
    };

    const click_handler_3 = function() {
      $$invalidate(1, telnet_input_field = "atsame5 bootloader");
      if (telnet_autosend)
        telnet_send();
    };

    const click_handler_4 = function() {
      $$invalidate(1, telnet_input_field = "atsame5 bootloader 16384");
      if (telnet_autosend)
        telnet_send();
    };

    const click_handler_5 = function() {
      $$invalidate(1, telnet_input_field = "atsame5 bootloader 0");
      if (telnet_autosend)
        telnet_send();
    };

    const click_handler_6 = function() {
      $$invalidate(
          1,
          telnet_input_field =
              "program bootloader-intech_grid-v3.3.0-8-g945e9ec-dirty.elf verify");
      if (telnet_autosend)
        telnet_send();
    };

    const click_handler_7 = function() {
      $$invalidate(1,
                   telnet_input_field =
                       "program ../grid_make/gcc/AtmelStart.bin verify 0x4000");
      if (telnet_autosend)
        telnet_send();
    };

    $$self.$$set = $$props => {
      if ("name" in $$props)
        $$invalidate(12, name = $$props.name);
    };

    $$self.$capture_state = () => ({
      onMount,
      element,
      name,
      uart_input_field,
      telnet_input_field,
      telnet_history,
      telnet_history_index,
      uart_console_enabled,
      openocd_console_enabled,
      telnet_console_enabled,
      telnet_autosend,
      keyup_telnet,
      fuser_kill,
      openocd_start,
      openocd_stop,
      telnet_start,
      telnet_stop,
      telnet_send,
      uart_send,
      consolePoll,
      ui_console,
      grid,
      mcu_pins
    });

    $$self.$inject_state = $$props => {
      if ("name" in $$props)
        $$invalidate(12, name = $$props.name);
      if ("uart_input_field" in $$props)
        $$invalidate(0, uart_input_field = $$props.uart_input_field);
      if ("telnet_input_field" in $$props)
        $$invalidate(1, telnet_input_field = $$props.telnet_input_field);
      if ("telnet_history" in $$props)
        telnet_history = $$props.telnet_history;
      if ("telnet_history_index" in $$props)
        telnet_history_index = $$props.telnet_history_index;
      if ("uart_console_enabled" in $$props)
        $$invalidate(2, uart_console_enabled = $$props.uart_console_enabled);
      if ("openocd_console_enabled" in $$props)
        $$invalidate(3,
                     openocd_console_enabled = $$props.openocd_console_enabled);
      if ("telnet_console_enabled" in $$props)
        $$invalidate(4,
                     telnet_console_enabled = $$props.telnet_console_enabled);
      if ("telnet_autosend" in $$props)
        $$invalidate(5, telnet_autosend = $$props.telnet_autosend);
      if ("ui_console" in $$props)
        $$invalidate(6, ui_console = $$props.ui_console);
      if ("grid" in $$props)
        $$invalidate(7, grid = $$props.grid);
      if ("mcu_pins" in $$props)
        $$invalidate(8, mcu_pins = $$props.mcu_pins);
    };

    if ($$props && "$$inject" in $$props) {
      $$self.$inject_state($$props.$$inject);
    }

    return [
      uart_input_field,
      telnet_input_field,
      uart_console_enabled,
      openocd_console_enabled,
      telnet_console_enabled,
      telnet_autosend,
      ui_console,
      grid,
      mcu_pins,
      keyup_telnet,
      telnet_send,
      uart_send,
      name,
      input5_input_handler,
      input7_input_handler,
      input9_change_handler,
      input10_change_handler,
      input11_change_handler,
      input12_change_handler,
      click_handler,
      click_handler_1,
      click_handler_2,
      click_handler_3,
      click_handler_4,
      click_handler_5,
      click_handler_6,
      click_handler_7
    ];
  }

  class App extends SvelteComponentDev {
    constructor(options) {
      super(options);
      init(this, options, instance, create_fragment, safe_not_equal,
           {name : 12}, [ -1, -1 ]);

      dispatch_dev("SvelteRegisterComponent", {
        component : this,
        tagName : "App",
        options,
        id : create_fragment.name
      });

      const {ctx} = this.$$;
      const props = options.props || {};

      if (/*name*/ ctx[12] === undefined && !("name" in props)) {
        console_1.warn("<App> was created without expected prop 'name'");
      }
    }

    get name() {
      throw new Error(
          "<App>: Props cannot be read directly from the component instance unless compiling with 'accessors: true' or '<svelte:options accessors/>'");
    }

    set name(value) {
      throw new Error(
          "<App>: Props cannot be set directly on the component instance unless compiling with 'accessors: true' or '<svelte:options accessors/>'");
    }
  }

  const app = new App({target : document.body, props : {name : 'world'}});

  return app;
}());
// # sourceMappingURL=bundle.js.map
