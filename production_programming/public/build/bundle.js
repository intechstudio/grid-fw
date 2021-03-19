
(function(l, r) { if (l.getElementById('livereloadscript')) return; r = l.createElement('script'); r.async = 1; r.src = '//' + (window.location.host || 'localhost').split(':')[0] + ':35730/livereload.js?snipver=1'; r.id = 'livereloadscript'; l.getElementsByTagName('head')[0].appendChild(r) })(window.document);
var app = (function () {
    'use strict';

    function noop() { }
    function add_location(element, file, line, column, char) {
        element.__svelte_meta = {
            loc: { file, line, column, char }
        };
    }
    function run(fn) {
        return fn();
    }
    function blank_object() {
        return Object.create(null);
    }
    function run_all(fns) {
        fns.forEach(run);
    }
    function is_function(thing) {
        return typeof thing === 'function';
    }
    function safe_not_equal(a, b) {
        return a != a ? b == b : a !== b || ((a && typeof a === 'object') || typeof a === 'function');
    }
    function is_empty(obj) {
        return Object.keys(obj).length === 0;
    }

    function append(target, node) {
        target.appendChild(node);
    }
    function insert(target, node, anchor) {
        target.insertBefore(node, anchor || null);
    }
    function detach(node) {
        node.parentNode.removeChild(node);
    }
    function destroy_each(iterations, detaching) {
        for (let i = 0; i < iterations.length; i += 1) {
            if (iterations[i])
                iterations[i].d(detaching);
        }
    }
    function element(name) {
        return document.createElement(name);
    }
    function text(data) {
        return document.createTextNode(data);
    }
    function space() {
        return text(' ');
    }
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
    function children(element) {
        return Array.from(element.childNodes);
    }
    function custom_event(type, detail) {
        const e = document.createEvent('CustomEvent');
        e.initCustomEvent(type, false, false, detail);
        return e;
    }

    let current_component;
    function set_current_component(component) {
        current_component = component;
    }
    function get_current_component() {
        if (!current_component)
            throw new Error('Function called outside component initialization');
        return current_component;
    }
    function onMount(fn) {
        get_current_component().$$.on_mount.push(fn);
    }

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
    function add_render_callback(fn) {
        render_callbacks.push(fn);
    }
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
            $$.dirty = [-1];
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

    const globals = (typeof window !== 'undefined'
        ? window
        : typeof globalThis !== 'undefined'
            ? globalThis
            : global);
    function mount_component(component, target, anchor, customElement) {
        const { fragment, on_mount, on_destroy, after_update } = component.$$;
        fragment && fragment.m(target, anchor);
        if (!customElement) {
            // onMount happens before the initial afterUpdate
            add_render_callback(() => {
                const new_on_destroy = on_mount.map(run).filter(is_function);
                if (on_destroy) {
                    on_destroy.push(...new_on_destroy);
                }
                else {
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
    function init(component, options, instance, create_fragment, not_equal, props, dirty = [-1]) {
        const parent_component = current_component;
        set_current_component(component);
        const $$ = component.$$ = {
            fragment: null,
            ctx: null,
            // state
            props,
            update: noop,
            not_equal,
            bound: blank_object(),
            // lifecycle
            on_mount: [],
            on_destroy: [],
            on_disconnect: [],
            before_update: [],
            after_update: [],
            context: new Map(parent_component ? parent_component.$$.context : []),
            // everything else
            callbacks: blank_object(),
            dirty,
            skip_bound: false
        };
        let ready = false;
        $$.ctx = instance
            ? instance(component, options.props || {}, (i, ret, ...rest) => {
                const value = rest.length ? rest[0] : ret;
                if ($$.ctx && not_equal($$.ctx[i], $$.ctx[i] = value)) {
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
            }
            else {
                // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
                $$.fragment && $$.fragment.c();
            }
            if (options.intro)
                transition_in(component.$$.fragment);
            mount_component(component, options.target, options.anchor, options.customElement);
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
            const callbacks = (this.$$.callbacks[type] || (this.$$.callbacks[type] = []));
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
        document.dispatchEvent(custom_event(type, Object.assign({ version: '3.35.0' }, detail)));
    }
    function append_dev(target, node) {
        dispatch_dev('SvelteDOMInsert', { target, node });
        append(target, node);
    }
    function insert_dev(target, node, anchor) {
        dispatch_dev('SvelteDOMInsert', { target, node, anchor });
        insert(target, node, anchor);
    }
    function detach_dev(node) {
        dispatch_dev('SvelteDOMRemove', { node });
        detach(node);
    }
    function listen_dev(node, event, handler, options, has_prevent_default, has_stop_propagation) {
        const modifiers = options === true ? ['capture'] : options ? Array.from(Object.keys(options)) : [];
        if (has_prevent_default)
            modifiers.push('preventDefault');
        if (has_stop_propagation)
            modifiers.push('stopPropagation');
        dispatch_dev('SvelteDOMAddEventListener', { node, event, handler, modifiers });
        const dispose = listen(node, event, handler, options);
        return () => {
            dispatch_dev('SvelteDOMRemoveEventListener', { node, event, handler, modifiers });
            dispose();
        };
    }
    function attr_dev(node, attribute, value) {
        attr(node, attribute, value);
        if (value == null)
            dispatch_dev('SvelteDOMRemoveAttribute', { node, attribute });
        else
            dispatch_dev('SvelteDOMSetAttribute', { node, attribute, value });
    }
    function set_data_dev(text, data) {
        data = '' + data;
        if (text.wholeText === data)
            return;
        dispatch_dev('SvelteDOMSetData', { node: text, data });
        text.data = data;
    }
    function validate_each_argument(arg) {
        if (typeof arg !== 'string' && !(arg && typeof arg === 'object' && 'length' in arg)) {
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
     * Base class for Svelte components with some minor dev-enhancements. Used when dev=true.
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
                console.warn('Component was already destroyed'); // eslint-disable-line no-console
            };
        }
        $capture_state() { }
        $inject_state() { }
    }

    /* svelte/App.svelte generated by Svelte v3.35.0 */

    const { console: console_1 } = globals;
    const file = "svelte/App.svelte";

    function get_each_context(ctx, list, i) {
    	const child_ctx = ctx.slice();
    	child_ctx[5] = list[i];
    	return child_ctx;
    }

    function get_each_context_1(ctx, list, i) {
    	const child_ctx = ctx.slice();
    	child_ctx[8] = list[i];
    	return child_ctx;
    }

    function get_each_context_2(ctx, list, i) {
    	const child_ctx = ctx.slice();
    	child_ctx[8] = list[i];
    	return child_ctx;
    }

    function get_each_context_3(ctx, list, i) {
    	const child_ctx = ctx.slice();
    	child_ctx[8] = list[i];
    	return child_ctx;
    }

    function get_each_context_4(ctx, list, i) {
    	const child_ctx = ctx.slice();
    	child_ctx[8] = list[i];
    	return child_ctx;
    }

    // (275:3) {#each mcu_pins[0] as pin}
    function create_each_block_4(ctx) {
    	let div;
    	let t_value = /*pin*/ ctx[8].number + "";
    	let t;
    	let div_class_value;

    	const block = {
    		c: function create() {
    			div = element("div");
    			t = text(t_value);
    			attr_dev(div, "class", div_class_value = "pin " + /*pin*/ ctx[8].class + " svelte-1ba0iqz");
    			add_location(div, file, 275, 4, 7536);
    		},
    		m: function mount(target, anchor) {
    			insert_dev(target, div, anchor);
    			append_dev(div, t);
    		},
    		p: function update(ctx, dirty) {
    			if (dirty & /*mcu_pins*/ 4 && t_value !== (t_value = /*pin*/ ctx[8].number + "")) set_data_dev(t, t_value);

    			if (dirty & /*mcu_pins*/ 4 && div_class_value !== (div_class_value = "pin " + /*pin*/ ctx[8].class + " svelte-1ba0iqz")) {
    				attr_dev(div, "class", div_class_value);
    			}
    		},
    		d: function destroy(detaching) {
    			if (detaching) detach_dev(div);
    		}
    	};

    	dispatch_dev("SvelteRegisterBlock", {
    		block,
    		id: create_each_block_4.name,
    		type: "each",
    		source: "(275:3) {#each mcu_pins[0] as pin}",
    		ctx
    	});

    	return block;
    }

    // (280:2) {#each mcu_pins[1] as pin}
    function create_each_block_3(ctx) {
    	let div;
    	let t_value = /*pin*/ ctx[8].number + "";
    	let t;
    	let div_class_value;

    	const block = {
    		c: function create() {
    			div = element("div");
    			t = text(t_value);
    			attr_dev(div, "class", div_class_value = "pin " + /*pin*/ ctx[8].class + " svelte-1ba0iqz");
    			add_location(div, file, 280, 3, 7663);
    		},
    		m: function mount(target, anchor) {
    			insert_dev(target, div, anchor);
    			append_dev(div, t);
    		},
    		p: function update(ctx, dirty) {
    			if (dirty & /*mcu_pins*/ 4 && t_value !== (t_value = /*pin*/ ctx[8].number + "")) set_data_dev(t, t_value);

    			if (dirty & /*mcu_pins*/ 4 && div_class_value !== (div_class_value = "pin " + /*pin*/ ctx[8].class + " svelte-1ba0iqz")) {
    				attr_dev(div, "class", div_class_value);
    			}
    		},
    		d: function destroy(detaching) {
    			if (detaching) detach_dev(div);
    		}
    	};

    	dispatch_dev("SvelteRegisterBlock", {
    		block,
    		id: create_each_block_3.name,
    		type: "each",
    		source: "(280:2) {#each mcu_pins[1] as pin}",
    		ctx
    	});

    	return block;
    }

    // (287:3) {#each mcu_pins[2] as pin}
    function create_each_block_2(ctx) {
    	let div;
    	let t_value = /*pin*/ ctx[8].number + "";
    	let t;
    	let div_class_value;

    	const block = {
    		c: function create() {
    			div = element("div");
    			t = text(t_value);
    			attr_dev(div, "class", div_class_value = "pin " + /*pin*/ ctx[8].class + " svelte-1ba0iqz");
    			add_location(div, file, 287, 4, 7795);
    		},
    		m: function mount(target, anchor) {
    			insert_dev(target, div, anchor);
    			append_dev(div, t);
    		},
    		p: function update(ctx, dirty) {
    			if (dirty & /*mcu_pins*/ 4 && t_value !== (t_value = /*pin*/ ctx[8].number + "")) set_data_dev(t, t_value);

    			if (dirty & /*mcu_pins*/ 4 && div_class_value !== (div_class_value = "pin " + /*pin*/ ctx[8].class + " svelte-1ba0iqz")) {
    				attr_dev(div, "class", div_class_value);
    			}
    		},
    		d: function destroy(detaching) {
    			if (detaching) detach_dev(div);
    		}
    	};

    	dispatch_dev("SvelteRegisterBlock", {
    		block,
    		id: create_each_block_2.name,
    		type: "each",
    		source: "(287:3) {#each mcu_pins[2] as pin}",
    		ctx
    	});

    	return block;
    }

    // (292:3) {#each mcu_pins[3] as pin}
    function create_each_block_1(ctx) {
    	let div;
    	let t_value = /*pin*/ ctx[8].number + "";
    	let t;
    	let div_class_value;

    	const block = {
    		c: function create() {
    			div = element("div");
    			t = text(t_value);
    			attr_dev(div, "class", div_class_value = "pin " + /*pin*/ ctx[8].class + " svelte-1ba0iqz");
    			add_location(div, file, 292, 4, 7925);
    		},
    		m: function mount(target, anchor) {
    			insert_dev(target, div, anchor);
    			append_dev(div, t);
    		},
    		p: function update(ctx, dirty) {
    			if (dirty & /*mcu_pins*/ 4 && t_value !== (t_value = /*pin*/ ctx[8].number + "")) set_data_dev(t, t_value);

    			if (dirty & /*mcu_pins*/ 4 && div_class_value !== (div_class_value = "pin " + /*pin*/ ctx[8].class + " svelte-1ba0iqz")) {
    				attr_dev(div, "class", div_class_value);
    			}
    		},
    		d: function destroy(detaching) {
    			if (detaching) detach_dev(div);
    		}
    	};

    	dispatch_dev("SvelteRegisterBlock", {
    		block,
    		id: create_each_block_1.name,
    		type: "each",
    		source: "(292:3) {#each mcu_pins[3] as pin}",
    		ctx
    	});

    	return block;
    }

    // (303:2) {#each [...ui_console.data].reverse() as entry}
    function create_each_block(ctx) {
    	let div;
    	let t_value = /*entry*/ ctx[5] + "";
    	let t;

    	const block = {
    		c: function create() {
    			div = element("div");
    			t = text(t_value);
    			attr_dev(div, "class", "consoleline svelte-1ba0iqz");
    			add_location(div, file, 303, 2, 8141);
    		},
    		m: function mount(target, anchor) {
    			insert_dev(target, div, anchor);
    			append_dev(div, t);
    		},
    		p: function update(ctx, dirty) {
    			if (dirty & /*ui_console*/ 1 && t_value !== (t_value = /*entry*/ ctx[5] + "")) set_data_dev(t, t_value);
    		},
    		d: function destroy(detaching) {
    			if (detaching) detach_dev(div);
    		}
    	};

    	dispatch_dev("SvelteRegisterBlock", {
    		block,
    		id: create_each_block.name,
    		type: "each",
    		source: "(303:2) {#each [...ui_console.data].reverse() as entry}",
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
    	let t1_value = /*grid*/ ctx[1].mcu + "";
    	let t1;
    	let td2;
    	let t2_value = /*grid*/ ctx[1].mcustatus + "";
    	let t2;
    	let t3;
    	let tr1;
    	let td3;
    	let td4;
    	let t5_value = /*grid*/ ctx[1].hwcfg + "";
    	let t5;
    	let td5;
    	let t6_value = /*grid*/ ctx[1].hwcfgstatus + "";
    	let t6;
    	let t7;
    	let tr2;
    	let td6;
    	let td7;
    	let t9_value = /*grid*/ ctx[1].model + "";
    	let t9;
    	let t10;
    	let tr3;
    	let td8;
    	let td9;
    	let t12_value = /*grid*/ ctx[1].serialno[0] + "";
    	let t12;
    	let td10;
    	let t13_value = /*grid*/ ctx[1].serialno[1] + "";
    	let t13;
    	let t14;
    	let tr4;
    	let td11;
    	let td12;
    	let t15_value = /*grid*/ ctx[1].serialno[2] + "";
    	let t15;
    	let td13;
    	let t16_value = /*grid*/ ctx[1].serialno[3] + "";
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
    	let t23;
    	let t24;
    	let input;
    	let mounted;
    	let dispose;
    	let each_value_4 = /*mcu_pins*/ ctx[2][0];
    	validate_each_argument(each_value_4);
    	let each_blocks_4 = [];

    	for (let i = 0; i < each_value_4.length; i += 1) {
    		each_blocks_4[i] = create_each_block_4(get_each_context_4(ctx, each_value_4, i));
    	}

    	let each_value_3 = /*mcu_pins*/ ctx[2][1];
    	validate_each_argument(each_value_3);
    	let each_blocks_3 = [];

    	for (let i = 0; i < each_value_3.length; i += 1) {
    		each_blocks_3[i] = create_each_block_3(get_each_context_3(ctx, each_value_3, i));
    	}

    	let each_value_2 = /*mcu_pins*/ ctx[2][2];
    	validate_each_argument(each_value_2);
    	let each_blocks_2 = [];

    	for (let i = 0; i < each_value_2.length; i += 1) {
    		each_blocks_2[i] = create_each_block_2(get_each_context_2(ctx, each_value_2, i));
    	}

    	let each_value_1 = /*mcu_pins*/ ctx[2][3];
    	validate_each_argument(each_value_1);
    	let each_blocks_1 = [];

    	for (let i = 0; i < each_value_1.length; i += 1) {
    		each_blocks_1[i] = create_each_block_1(get_each_context_1(ctx, each_value_1, i));
    	}

    	let each_value = [.../*ui_console*/ ctx[0].data].reverse();
    	validate_each_argument(each_value);
    	let each_blocks = [];

    	for (let i = 0; i < each_value.length; i += 1) {
    		each_blocks[i] = create_each_block(get_each_context(ctx, each_value, i));
    	}

    	const block = {
    		c: function create() {
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
    			div7.textContent = "Debug UART Console:";
    			t23 = space();

    			for (let i = 0; i < each_blocks.length; i += 1) {
    				each_blocks[i].c();
    			}

    			t24 = space();
    			input = element("input");
    			add_location(td0, file, 250, 6, 7014);
    			add_location(td1, file, 250, 20, 7028);
    			add_location(td2, file, 250, 39, 7047);
    			add_location(tr0, file, 249, 5, 7003);
    			add_location(td3, file, 254, 6, 7102);
    			add_location(td4, file, 254, 21, 7117);
    			add_location(td5, file, 254, 42, 7138);
    			add_location(tr1, file, 253, 5, 7091);
    			add_location(td6, file, 257, 6, 7193);
    			attr_dev(td7, "colspan", "2");
    			add_location(td7, file, 257, 21, 7208);
    			add_location(tr2, file, 256, 5, 7182);
    			add_location(td8, file, 261, 6, 7270);
    			add_location(td9, file, 261, 19, 7283);
    			add_location(td10, file, 261, 46, 7310);
    			add_location(tr3, file, 260, 5, 7259);
    			add_location(td11, file, 264, 6, 7365);
    			add_location(td12, file, 264, 15, 7374);
    			add_location(td13, file, 264, 42, 7401);
    			add_location(tr4, file, 263, 5, 7354);
    			attr_dev(table, "border", "1");
    			attr_dev(table, "width", "300px");
    			add_location(table, file, 247, 4, 6964);
    			attr_dev(div0, "class", "chip_info svelte-1ba0iqz");
    			add_location(div0, file, 246, 6, 6936);
    			attr_dev(div1, "class", "chip svelte-1ba0iqz");
    			add_location(div1, file, 244, 2, 6910);
    			attr_dev(div2, "class", "side rot0 svelte-1ba0iqz");
    			add_location(div2, file, 273, 2, 7478);
    			attr_dev(div3, "class", "side rot90 svelte-1ba0iqz");
    			add_location(div3, file, 278, 2, 7606);
    			attr_dev(div4, "class", "side rot180 svelte-1ba0iqz");
    			add_location(div4, file, 285, 2, 7735);
    			attr_dev(div5, "class", "side rot270 svelte-1ba0iqz");
    			add_location(div5, file, 290, 2, 7865);
    			attr_dev(div6, "class", "boundary_check container svelte-1ba0iqz");
    			add_location(div6, file, 242, 1, 6868);
    			attr_dev(div7, "class", "consoletitle svelte-1ba0iqz");
    			add_location(div7, file, 301, 2, 8037);
    			attr_dev(div8, "class", "serial_console svelte-1ba0iqz");
    			add_location(div8, file, 300, 1, 8006);
    			attr_dev(input, "type", "button");
    			input.value = "Start OpenOCD";
    			add_location(input, file, 308, 0, 8202);
    			attr_dev(main, "class", "svelte-1ba0iqz");
    			add_location(main, file, 240, 0, 6859);
    		},
    		l: function claim(nodes) {
    			throw new Error("options.hydrate only works if the component was compiled with the `hydratable: true` option");
    		},
    		m: function mount(target, anchor) {
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
    			append_dev(div8, t23);

    			for (let i = 0; i < each_blocks.length; i += 1) {
    				each_blocks[i].m(div8, null);
    			}

    			append_dev(main, t24);
    			append_dev(main, input);

    			if (!mounted) {
    				dispose = listen_dev(input, "click", openocd_get_version, false, false, false);
    				mounted = true;
    			}
    		},
    		p: function update(ctx, [dirty]) {
    			if (dirty & /*grid*/ 2 && t1_value !== (t1_value = /*grid*/ ctx[1].mcu + "")) set_data_dev(t1, t1_value);
    			if (dirty & /*grid*/ 2 && t2_value !== (t2_value = /*grid*/ ctx[1].mcustatus + "")) set_data_dev(t2, t2_value);
    			if (dirty & /*grid*/ 2 && t5_value !== (t5_value = /*grid*/ ctx[1].hwcfg + "")) set_data_dev(t5, t5_value);
    			if (dirty & /*grid*/ 2 && t6_value !== (t6_value = /*grid*/ ctx[1].hwcfgstatus + "")) set_data_dev(t6, t6_value);
    			if (dirty & /*grid*/ 2 && t9_value !== (t9_value = /*grid*/ ctx[1].model + "")) set_data_dev(t9, t9_value);
    			if (dirty & /*grid*/ 2 && t12_value !== (t12_value = /*grid*/ ctx[1].serialno[0] + "")) set_data_dev(t12, t12_value);
    			if (dirty & /*grid*/ 2 && t13_value !== (t13_value = /*grid*/ ctx[1].serialno[1] + "")) set_data_dev(t13, t13_value);
    			if (dirty & /*grid*/ 2 && t15_value !== (t15_value = /*grid*/ ctx[1].serialno[2] + "")) set_data_dev(t15, t15_value);
    			if (dirty & /*grid*/ 2 && t16_value !== (t16_value = /*grid*/ ctx[1].serialno[3] + "")) set_data_dev(t16, t16_value);

    			if (dirty & /*mcu_pins*/ 4) {
    				each_value_4 = /*mcu_pins*/ ctx[2][0];
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

    			if (dirty & /*mcu_pins*/ 4) {
    				each_value_3 = /*mcu_pins*/ ctx[2][1];
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

    			if (dirty & /*mcu_pins*/ 4) {
    				each_value_2 = /*mcu_pins*/ ctx[2][2];
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

    			if (dirty & /*mcu_pins*/ 4) {
    				each_value_1 = /*mcu_pins*/ ctx[2][3];
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

    			if (dirty & /*ui_console*/ 1) {
    				each_value = [.../*ui_console*/ ctx[0].data].reverse();
    				validate_each_argument(each_value);
    				let i;

    				for (i = 0; i < each_value.length; i += 1) {
    					const child_ctx = get_each_context(ctx, each_value, i);

    					if (each_blocks[i]) {
    						each_blocks[i].p(child_ctx, dirty);
    					} else {
    						each_blocks[i] = create_each_block(child_ctx);
    						each_blocks[i].c();
    						each_blocks[i].m(div8, null);
    					}
    				}

    				for (; i < each_blocks.length; i += 1) {
    					each_blocks[i].d(1);
    				}

    				each_blocks.length = each_value.length;
    			}
    		},
    		i: noop,
    		o: noop,
    		d: function destroy(detaching) {
    			if (detaching) detach_dev(main);
    			destroy_each(each_blocks_4, detaching);
    			destroy_each(each_blocks_3, detaching);
    			destroy_each(each_blocks_2, detaching);
    			destroy_each(each_blocks_1, detaching);
    			destroy_each(each_blocks, detaching);
    			mounted = false;
    			dispose();
    		}
    	};

    	dispatch_dev("SvelteRegisterBlock", {
    		block,
    		id: create_fragment.name,
    		type: "component",
    		source: "",
    		ctx
    	});

    	return block;
    }

    async function openocd_get_version() {
    	try {
    		const serial = await fetch("/api/openocd/version", { method: "GET" }).then(res => res.json());
    		console.log(serial);
    	} catch(error) {
    		
    	}
    }

    function instance($$self, $$props, $$invalidate) {
    	let { $$slots: slots = {}, $$scope } = $$props;
    	validate_slots("App", slots, []);
    	let { name } = $$props;

    	async function serialInterval() {
    		try {
    			const serial = await fetch("/api/serial", { method: "GET" }).then(res => res.json());
    			console.log(serial);

    			serial.forEach(element => {
    				//var str = String.fromCharCode.apply(null, element.data);
    				console.log(element);

    				$$invalidate(0, ui_console.data = [...ui_console.data, element], ui_console);
    				let str = element;

    				if (str.startsWith("test")) {
    					if (str.startsWith("test.boundary.")) {
    						var test_result = str.substring(16, 16 + 25);
    						console.log("Boundary Test Result is Here!");
    						console.log(parseInt(str.substring(14, 15)));
    						console.log(test_result);
    						var indices = [];

    						for (var i = 0; i < test_result.length; i++) {
    							if (test_result[i] === "1") indices.push(i);
    						}

    						console.log(indices);

    						for (var i = 0; i < indices.length; i++) {
    							let side = parseInt(str.substring(14, 15));
    							$$invalidate(2, mcu_pins[side][indices[i]].class += "pinerror", mcu_pins);
    						}
    					} else if (str.startsWith("test.hwcfg.")) {
    						$$invalidate(1, grid.hwcfg = parseInt(str.substring(11, str.length)), grid);

    						if (grid.hwcfg == 192) {
    							$$invalidate(1, grid.model = "EN16 RevA", grid);
    							$$invalidate(1, grid.hwcfgstatus = "OK", grid);
    						}
    					} else if (str.startsWith("test.serialno.")) {
    						$$invalidate(1, grid.serialno = str.substring(14, str.length).split(" "), grid);
    					} else if (str.startsWith("test.mcu.")) {
    						$$invalidate(1, grid.mcu = str.split(".")[2], grid);

    						if (grid.mcu == "ATSAMD51N20A") {
    							$$invalidate(1, grid.mcustatus = "OK", grid);
    						}
    					}
    				}
    			});
    		} catch(error) {
    			
    		}
    	}

    	

    	onMount(() => {
    		setInterval(serialInterval, 1000);
    	});

    	let ui_console = { data: [] };

    	let grid = {
    		hwcfg: "???",
    		hwcfgstatus: "?",
    		mcu: "?",
    		mcustatus: "?",
    		model: "???",
    		serialno: ["?", "?", "?", "?"]
    	};

    	let mcu_pins = [
    		[
    			{ number: "1", function: "none", class: "" },
    			{ number: "2", function: "none", class: "" },
    			{ number: "3", function: "none", class: "" },
    			{ number: "4", function: "none", class: "" },
    			{ number: "5", function: "none", class: "" },
    			{ number: "6", function: "none", class: "" },
    			{ number: "7", function: "none", class: "" },
    			{ number: "8", function: "none", class: "" },
    			{ number: "9", function: "none", class: "" },
    			{
    				number: "10",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "11",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "12",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "13",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "14",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "15",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "16",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "17",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "18",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "19",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "20",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "21",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "22",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "23",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "24",
    				function: "none",
    				class: ""
    			},
    			{ number: "25", function: "none" }
    		],
    		[
    			{
    				number: "26",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "27",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "28",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "29",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "30",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "31",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "32",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "33",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "34",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "35",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "36",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "37",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "38",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "39",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "40",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "41",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "42",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "43",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "44",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "45",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "46",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "47",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "48",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "49",
    				function: "none",
    				class: ""
    			},
    			{ number: "50", function: "none" }
    		],
    		[
    			{
    				number: "51",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "52",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "53",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "54",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "55",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "56",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "57",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "58",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "59",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "60",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "61",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "62",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "63",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "64",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "65",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "66",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "67",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "68",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "69",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "70",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "71",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "72",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "73",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "74",
    				function: "none",
    				class: ""
    			},
    			{ number: "75", function: "none" }
    		],
    		[
    			{
    				number: "76",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "77",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "78",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "79",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "80",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "81",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "82",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "83",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "84",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "85",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "86",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "87",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "88",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "89",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "90",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "91",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "92",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "93",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "94",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "95",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "96",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "97",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "98",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "99",
    				function: "none",
    				class: ""
    			},
    			{
    				number: "100",
    				function: "none",
    				class: ""
    			}
    		]
    	];

    	const writable_props = ["name"];

    	Object.keys($$props).forEach(key => {
    		if (!~writable_props.indexOf(key) && key.slice(0, 2) !== "$$") console_1.warn(`<App> was created with unknown prop '${key}'`);
    	});

    	$$self.$$set = $$props => {
    		if ("name" in $$props) $$invalidate(3, name = $$props.name);
    	};

    	$$self.$capture_state = () => ({
    		onMount,
    		name,
    		openocd_get_version,
    		serialInterval,
    		ui_console,
    		grid,
    		mcu_pins
    	});

    	$$self.$inject_state = $$props => {
    		if ("name" in $$props) $$invalidate(3, name = $$props.name);
    		if ("ui_console" in $$props) $$invalidate(0, ui_console = $$props.ui_console);
    		if ("grid" in $$props) $$invalidate(1, grid = $$props.grid);
    		if ("mcu_pins" in $$props) $$invalidate(2, mcu_pins = $$props.mcu_pins);
    	};

    	if ($$props && "$$inject" in $$props) {
    		$$self.$inject_state($$props.$$inject);
    	}

    	return [ui_console, grid, mcu_pins, name];
    }

    class App extends SvelteComponentDev {
    	constructor(options) {
    		super(options);
    		init(this, options, instance, create_fragment, safe_not_equal, { name: 3 });

    		dispatch_dev("SvelteRegisterComponent", {
    			component: this,
    			tagName: "App",
    			options,
    			id: create_fragment.name
    		});

    		const { ctx } = this.$$;
    		const props = options.props || {};

    		if (/*name*/ ctx[3] === undefined && !("name" in props)) {
    			console_1.warn("<App> was created without expected prop 'name'");
    		}
    	}

    	get name() {
    		throw new Error("<App>: Props cannot be read directly from the component instance unless compiling with 'accessors: true' or '<svelte:options accessors/>'");
    	}

    	set name(value) {
    		throw new Error("<App>: Props cannot be set directly on the component instance unless compiling with 'accessors: true' or '<svelte:options accessors/>'");
    	}
    }

    const app = new App({
    	target: document.body,
    	props: {
    		name: 'world'
    	}
    });

    return app;

}());
//# sourceMappingURL=bundle.js.map
